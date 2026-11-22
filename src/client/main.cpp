#include "client/main.hpp"
#include "client/receiver.hpp"
#include "client/connector.hpp"
#include "client/command_parser.hpp"
#include "client/history.hpp"
#include "common/config.hpp"
#include "common/protocol.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include "common/helpers.hpp"
#include "client/history.hpp"

int main(int argc, char **argv) {
  auto cfg = common::parse_args(argc, argv);
  if (cfg.host.empty() || cfg.port == 0) {
    return 1;
  }
  std::string host = cfg.host;
  int port = cfg.port;
  
  client::Connector connector;
  if(!connector.connect_to(host,port)){
    std::cerr << "connect to (" << host << ":"<<port<<") failed\n";
    return 1;
  }
  if(!connector.handshake()){
    std::cerr << "handshake failed\n";
    return 1;
  }

  std::string nickname;

  for (;;) {
    std::cout << "\nPodaj nick: ";
    std::getline(std::cin,nickname);

    const std::string nick_cmd = "NICK " + nickname;
    connector.send_line("NICK "+nickname);
    client::history_append(nick_cmd);

    std::string respond;
    bool closed = false;

    if(!connector.recv_line(respond,closed)){
      std::cerr << "Utracono połączenie z serwerem.\n";
      return 1;
    }

    if(respond.rfind(common::OK{common::CODE_200_OK, ""}.to_line(),0) == 0){
      std::cout << "\nZalogowano jako " << nickname << "\n";
      break;
    }
    else if(respond.rfind(common::ERR{common::CODE_409_CONFLICT,""}.to_line(),0) == 0){
      std::cout << "\nNick zajęty: " << respond << "\n";
    }
    else if(respond.rfind(common::ERR{common::CODE_400_BAD_REQUEST,""}.to_line(),0) == 0){
      std::cout << "\nNiepoprawny format nicku: " << respond << "\n";
    }
    else {
      
      std::cout << "\nNieznana odpowiedź serwera: " << respond << "\n";
    }
    
  }
  std::atomic<bool> running{true};

  std::thread receiver_thread = client::start_receiver(connector, running);

  std::string input;
  while (running) {
      std::cout << ">";
      if (!std::getline(std::cin, input)) {
          break;
      }
      if (input.empty()) continue;

      if (input[0] == '/') {
          std::string proto = client::to_protocol(input.substr(1));
          if (proto.empty()) {
              std::cout << "Niepoprawna komenda.\n";
              continue;
          }

          connector.send_line(proto);
          client::history_append(proto);

          // if (proto == "QUIT") {
          //     running = false;
          //     break;
          // }
      } else {
          std::cout << "\nKomenda powinna zaczynać się od /\n";
      }
  }
  running = false;
  connector.disconnect();
  if(receiver_thread.joinable()) receiver_thread.join();

  return 0;
}
