#include "common/helpers.hpp"
#include "common/time_utils.hpp"
#include "server/listener.hpp"
#include "server/logger.hpp"
#include "server/client_session.hpp"
#include "server/room_manager.hpp"
#include "server/dispatcher.hpp"
#include "common/config.hpp"
#include "common/protocol.hpp"
#include <unordered_map>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <sys/socket.h>
#include <unistd.h>
#include <cctype>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <vector>
#include <atomic>
#include <stdexcept>
int main(int argc, char **argv) {

  auto cfg = common::parse_args(argc, argv);
  if (cfg.host.empty() || cfg.port == 0) {
    return 1;
  }
  std::string host = cfg.host;
  int port = cfg.port;

  
  std::filesystem::create_directories("./data");
  std::filesystem::create_directories("./logs");

  const std::string rooms_path = "./data/rooms.txt";
  const std::string log_path   = "./logs/server.log";

  server::Logger logger;
  logger.open(log_path);

  server::Listener listener;
  if(!listener.init(host,port)){
    std::cerr << "Listener init failure";
    return 1;
  }

  server::UserRegistry user_registry;

  std::mutex rooms_mx;
  server::RoomManager rooms(rooms_path);
  rooms.load_from_file();
  rooms.ensure_default_general();
  rooms.save_to_file();

  server::Dispatcher dispatcher(rooms, rooms_mx, user_registry, logger);
  
  logger.log("Server started, listening on "+std::to_string(port));

  std::thread ping_thread([&user_registry, &dispatcher]() {
    using namespace std::chrono_literals;
    const auto CHECK         = 1s;
    const auto PING_INTERVAL = 30s;
    const auto TIMEOUT       = 60s;
    
    
    for (;;) {
        std::this_thread::sleep_for(CHECK);
        auto now = std::chrono::steady_clock::now();

        auto sessions = user_registry.snapshot_sessions();

        for (auto& sess : sessions) {
            if (!sess) {
                continue;
            }

            if (sess->ping_timed_out(now, std::chrono::seconds(TIMEOUT))) {
              //dispatcher.handle_disconnect(*sess);
              sess->send_line( common::ERR{common::CODE_408_PING_TIMEOUT,common::ERR_PING_TIMEOUT}.to_line() );
              sess->force_close(); 
            } else {

              if (!sess->is_waiting_for_pong()) {
                  if ((now - sess->last_ping_time()) >= PING_INTERVAL) {
                      sess->mark_ping_sent(now);
                      sess->send_line("PING");
                  }
              }
              
              
            }
        }
    }
  });

  ping_thread.detach();

  for (;;) {
    int cfd = ::accept(listener.get_socket(), nullptr, nullptr);
    if (cfd < 0) {
        continue;
    }

    std::thread([cfd, &logger, &user_registry, &rooms, &rooms_mx,&dispatcher]() {
      std::shared_ptr<server::ClientSession> client_ptr = std::make_shared<server::ClientSession>(cfd);
      server::ClientSession& client = *client_ptr;

      std::string line;
      bool conn_closed = false;

      if(!client.recv_line(line,conn_closed)){
        ::close(cfd);
        return;
      }
      if(line != common::HELLO) {//zobacz czy siei zgadza
        
        client.send_line(common::ERR{common::CODE_400_BAD_REQUEST,common::UNSUPPORTED_PROTOCOL}.to_line());
        ::close(cfd);
        return;
      }
      client.send_line(common::WELCOME);
      client.authenticated = true;
      
      std::string nickname_line;
      for(;;){
        
        if(!client.recv_line(nickname_line,conn_closed)){
          break;//błąd albo przerwanie przerwanie
        }
        if(nickname_line.rfind("NICK ",0) == 0){
          std::string nickname = nickname_line.substr(5);

          bool valid = std::all_of(nickname.begin(),nickname.end(),
            [](unsigned char c){ return std::isalnum(c) or c == '_';}
          );
          
          if(nickname.size() < 3 or nickname.size() > 16 or !valid){
            std::cout << "Nick musi zawierać 3 - 16 znaków.\n";
            client.send_line(common::ERR{common::CODE_400_BAD_REQUEST,common::ERR_MSG_INVALID_NICKNAME}.to_line());
            continue;
          }
         
          if (user_registry.has_user(nickname)) {
              client.send_line(common::ERR{
                  common::CODE_409_CONFLICT,
                  common::ERR_MSG_NICK_IN_USE
              }.to_line());
              continue;
          }

          if (!user_registry.add_user(nickname, client_ptr)) {
              client.send_line(common::ERR{
                  common::CODE_409_CONFLICT,
                  common::ERR_MSG_NICK_IN_USE
              }.to_line());
              continue;
          }
          client.nickname = nickname;

          logger.log("[INFO] User "+ nickname+ " logged in (fd="+ std::to_string(cfd) + ")");
          client.send_line(common::OK{common::CODE_200_OK, common::OK_MSG_NICK_SET}.to_line());
          client.mark_pong_received();
          break;
          
        } else {
          std::cerr << "Nieoczekiwana komenda przed NICK: " << nickname_line << "\n";
          client.send_line(common::ERR{common::CODE_401_UNAUTHORIZED, common::ERR_MSG_SYNTAX_ERROR}.to_line());
          dispatcher.handle_disconnect(client);
          ::close(cfd);
          return;
        }

      }
      
      for(;;){
        line.clear();
        if (!client.recv_line(line, conn_closed)) {
          //dispatcher.handle_disconnect(client);
          break;
        }
        bool keep_running = dispatcher.dispatch(client, line);
        if (!keep_running) {
            break;
        }
      }
      dispatcher.handle_disconnect(client);
      ::close(cfd);
    }).detach();
  }
  return 0;
}