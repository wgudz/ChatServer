#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <memory>

#include "server/client_session.hpp"
#include "server/room_manager.hpp"
#include "server/logger.hpp"
#include "server/user_registry.hpp"

namespace server {

class Dispatcher {
public:
    Dispatcher(RoomManager& rooms,
               std::mutex& rooms_mx,
               UserRegistry& users,
               server::Logger& logger);

    bool dispatch(ClientSession& client, const std::string& line);
    void handle_disconnect(ClientSession& client);

private:
    RoomManager& rooms_;
    std::mutex& rooms_mx_;
    UserRegistry& users_;
    server::Logger& logger_;

    void handle_quit(ClientSession& client,
                     const std::vector<std::string>& args);

    void handle_join(ClientSession& client,
                     const std::vector<std::string>& args);

    void handle_msg(ClientSession& client,
                    const std::string& rest);

    void handle_leave(ClientSession& client,
                      const std::vector<std::string>& args);

    void handle_priv(ClientSession& client,
                 const std::string& rest);

    void handle_list(ClientSession& client,
                     const std::vector<std::string>& args);

    void handle_ping(ClientSession& client);

    void broadcast_event_to_room(
        const std::string& room_name,
        const std::string& event_line);
    void handle_pong(ClientSession& client);
    void handle_kick(ClientSession& client,const std::vector<std::string>& args);
    void handle_mute(ClientSession& client,const std::vector<std::string>& args);
    void handle_status(ClientSession& client,const std::vector<std::string>& args);
    void handle_whoami(ClientSession& client);
                 
};

}