#include "server/dispatcher.hpp"
#include "common/protocol.hpp"
#include "common/net_utils.hpp"
#include "common/time_utils.hpp"
#include "common/helpers.hpp"

#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <iostream>

namespace server {

Dispatcher::Dispatcher(RoomManager& rooms,
                       std::mutex& rooms_mx,
                       UserRegistry& users,
                       server::Logger& logger)
    : rooms_(rooms)
    , rooms_mx_(rooms_mx)
    , users_(users)
    , logger_(logger)
{}

bool Dispatcher::dispatch(ClientSession& client, const std::string& line) {
    constexpr int MAX_CMDS_PER_SEC = 5;
    if (client.commands_last_second() > MAX_CMDS_PER_SEC) {
        client.send_line(common::ERR{
            common::CODE_429_TOO_MANY_REQUESTS,
            common::ERR_MSG_TOO_MANY_REQUESTS
        }.to_line());
        return true;
    }

    if (client.nickname.empty()) {
        client.send_line(common::ERR{
            common::CODE_401_UNAUTHORIZED,
            common::ERR_MSG_NOT_LOGGED_IN
        }.to_line());
        return true;
    }

    std::istringstream iss(line);
    std::string cmd;
    if (!(iss >> cmd)) {
        return true;
    }

    std::vector<std::string> args;
    std::string token;
    while (iss >> token) {
        args.push_back(token);
    }
    // dla msg i priv
    std::string rest;
    auto pos = line.find(' ');
    if (pos != std::string::npos) {
        rest = line.substr(pos + 1);
    } else {
        rest = "";
    }
    //
    if (cmd == "QUIT") {
        handle_quit(client, args);
        return false;
    } else if (cmd == "JOIN") {
        handle_join(client, args);
    } else if (cmd == "MSG") {
        handle_msg(client, rest);
    } else if (cmd == "LEAVE") {
        handle_leave(client, args);
    } else if (cmd == "PRIV") {
        handle_priv(client, rest);
    } else if (cmd == "LIST") {
        handle_list(client, args);
    } else if (cmd == "PING") {
        handle_ping(client);
    } else if (cmd == "PONG") {
        handle_pong(client);
    } else if (cmd == "KICK") {
        handle_kick(client,args);
    } else if (cmd == "MUTE") {
        handle_mute(client,args);
    } else if (cmd == "STATUS") {
        handle_status(client,args);
    }  else if (cmd == "WHOAMI") {
        handle_whoami(client);
    } else {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
    }
    return true;
}

void Dispatcher::handle_disconnect(ClientSession& client) {
    users_.remove_user(client.nickname);

    std::unordered_set<std::string> members_list;

    for (const std::string& room_name : client.get_joined_rooms()) {
        bool room_deleted = false;
        {
            std::lock_guard<std::mutex> lock(rooms_mx_);
            rooms_.remove_user_from_room(room_name, client.nickname);
            if (rooms_.room_empty(room_name)) {
                rooms_.delete_room(room_name);
                rooms_.save_to_file();
                room_deleted = true;
            }
        }

        if (room_deleted) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(rooms_mx_);
            members_list = rooms_.members(room_name);
        }

        for (const auto& member_nick : members_list) {
            auto client_member_ptr = users_.find_user(member_nick);
            if (!client_member_ptr) {
                continue;
            }

            client_member_ptr->send_line(
                "EVENT LEAVE " + room_name + " " +
                client.nickname + " " + common::timestamp()
            );
        }
    }

    logger_.log("User " + client.nickname + " disconnected");
}

void Dispatcher::handle_quit(ClientSession& client,
                             const std::vector<std::string>&) {
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::MSG_BYE
    }.to_line());

    //handle_disconnect(client);
}

void Dispatcher::handle_join(ClientSession& client,
                             const std::vector<std::string>& args) {
    if (args.size() != 1) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_SYNTAX_ERROR
        }.to_line());
        return;
    }

    std::string room_name = args[0];
    std::string room_body = room_name.substr(1);
    bool valid = std::all_of(room_body.begin(), room_body.end(),
        [](unsigned char c) { return std::isalnum(c) || c == '_'; });

    if (room_name.size() <= 1 || !valid || room_name[0] != '#') {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_INVALID_ROOM_NAME
        }.to_line());
        return;
    }

    bool room_created = false;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        std::cout << "Handle join" << std::endl;
        if (!rooms_.has_room(room_name)) {
            rooms_.create_room(room_name);
            rooms_.get_room(room_name)->moderator_nick = client.nickname;
            rooms_.save_to_file();
            room_created = true;
        }
    }

    if (room_created) {
        logger_.log("[INFO] Room created: " + room_name);
        //client.send_line("INFO Room has been created.");
        // client.send_line(common::OK{
        //     common::CODE_200_OK ,
        //     common::OK_MSG_ROOM_CREATED
        // }.to_line());
    }
    bool is_in_room = false;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        is_in_room = rooms_.is_user_in_room(room_name,client.nickname);
    }
    if (is_in_room) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ALREADY_IN_ROOM
        }.to_line());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        rooms_.add_user_to_room(room_name, client.nickname);
    }

    client.join_room(room_name);
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_MSG_JOINED_PREFIX + room_name
    }.to_line());
    
    // std::string history = "";
    // {
    //     std::lock_guard<std::mutex> lock(rooms_mx_);
    //     history += rooms_.get_history(room_name);
    // }
    // if (!history.empty()) {
    //     client.send_line("EVENT HISTORY\n--------\n" + history+"\n--------");
    // }
    
    std::string event =
        "EVENT JOIN " + room_name + " " +
        client.nickname + " " + common::timestamp();

    broadcast_event_to_room(room_name, event);
}

void Dispatcher::handle_msg(ClientSession& client,
    const std::string& rest) {
    
    std::vector<std::string> args;
    std::string word;
    std::istringstream iss(rest);
    while (iss >> word) {
        args.push_back(word);
    }
    
    if (args.size() < 2) {//pustą wiadomość uważasz za bład
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_SYNTAX_ERROR
        }.to_line());
        return;
    }

    std::string room_name = args[0];
    std::string message;

    auto pos = rest.find(' ');
    if (pos != std::string::npos)
        message = rest.substr(pos + 1);
    else
        message = "";
    
    bool is_in_room = false;
    // std::unordered_map<
    //     std::string,
    //     std::chrono::steady_clock::time_point
    // > mutes;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        is_in_room = rooms_.is_user_in_room(room_name, client.nickname);
        // if(is_in_room){
        //     mutes = rooms_.get_room(room_name) -> mutes;
        // }
        
    }

    if (!is_in_room) {
        client.send_line(common::ERR{
            common::CODE_403_FORBIDDEN,
            common::ERR_MSG_NOT_IN_ROOM
        }.to_line());
        return;
    }

    bool muted = false;
    long long remaining = 0;

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        Room* room_ptr = rooms_.get_room(room_name);
        if (room_ptr) {
            auto it = room_ptr->mutes.find(client.nickname);
            if (it != room_ptr->mutes.end()) {
                auto now = std::chrono::steady_clock::now();
                if (now < it->second) {
                    muted = true;
                    remaining = std::chrono::duration_cast<std::chrono::seconds>(it->second - now).count();
                } else {
                    room_ptr->mutes.erase(it);
                }
            }
        }
    }

    if (muted) {
        client.send_line(common::ERR{
            common::CODE_403_FORBIDDEN,
            common::MUTED_PREFIX + std::to_string(remaining) + "s"
        }.to_line());
        return;
    }

    std::string event_message =
        "EVENT MSG " + room_name + " " +
        client.nickname + " " + common::timestamp() +
        " " + message;

    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_MSG_DELIVERED
    }.to_line());

    broadcast_event_to_room(room_name, event_message);

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        rooms_.add_message(room_name, event_message);
        rooms_.append_history_to_disk(room_name);
    }
}
 
    
    


void Dispatcher::handle_leave(ClientSession& client,
                              const std::vector<std::string>& args) {
    if (args.size() != 1) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_SYNTAX_ERROR
        }.to_line());
        return;
    }

    std::string room_name = args[0];

    bool has_room = false;
    bool is_user_in_room = false;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        has_room        = rooms_.has_room(room_name);
        is_user_in_room = rooms_.is_user_in_room(room_name, client.nickname);
    }

    if (!has_room) {
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_NOT_FOUND
        }.to_line());
        return;
    }

    if (!is_user_in_room) {
        client.send_line(common::ERR{
            common::CODE_403_FORBIDDEN,
            common::ERR_MSG_NOT_IN_ROOM
        }.to_line());
        return;
    }

    bool room_empty = false;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        rooms_.remove_user_from_room(room_name, client.nickname);
        room_empty = rooms_.room_empty(room_name);
        if (room_empty) {
            rooms_.delete_room(room_name);
            rooms_.save_to_file();
        }
    }

    client.quit_room(room_name);
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_MSG_LEFT_PREFIX + room_name
    }.to_line());

    if (room_empty) {
        logger_.log("[INFO] Room " + room_name + " is now empty and was deleted");
        return;
    }

    std::string event_message =
        "EVENT LEAVE " + room_name + " " +
        client.nickname + " " + common::timestamp();

    broadcast_event_to_room(room_name, event_message);

    logger_.log("[INFO] User: " + client.nickname + " left room " + room_name);
}

void Dispatcher::handle_priv(ClientSession& client,
    const std::string& rest) {

    std::vector<std::string> args;
    std::string word;
    std::istringstream iss(rest);
    while (iss >> word) {
        args.push_back(word);
    }
    
    if (args.size() < 2) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_SYNTAX_ERROR
        }.to_line());
        return;
    }

    std::string receiver_nick = args[0];
    std::string message;

    auto pos = rest.find(' ');
    if (pos != std::string::npos)
        message = rest.substr(pos + 1);
    else
        message = "";
    
    auto receiver = users_.find_user(receiver_nick);

    if (!receiver) {
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_NOT_FOUND
        }.to_line());
        return;
    }

    std::string event_message =
        "EVENT PRIV " + client.nickname +
        " " + common::timestamp() + " " + message;

    logger_.log("User " + client.nickname +
                " -> " + receiver_nick +
                " (priv): " + message);

    receiver->send_line(event_message);
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_MSG_DELIVERED
    }.to_line());
}
void Dispatcher::handle_mute(ClientSession& client, const std::vector<std::string>& args)
{
    if (args.size() != 3) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
        return;
    }

    const std::string room_name = args[0];
    const std::string nick = args[1];

    int time = 0;
    try {
        size_t pos = 0;
        time = std::stoi(args[2], &pos);
        if (pos != args[2].size()) { 
            throw std::invalid_argument("not a pure int");
        }
    } catch (...) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
        return;
    }

    if (time <= 0) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
        return;
    }

    std::string moderator;
    bool has_room = false;
    bool is_target_exist = false;

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        has_room = rooms_.has_room(room_name);
        if (has_room) {
            moderator = rooms_.get_room(room_name)->moderator_nick;
            is_target_exist = rooms_.is_user_in_room(room_name, nick);
        }
    }

    if (!has_room) {
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_INVALID_ROOM_NAME
        }.to_line());
        return;
    }

    if (!is_target_exist) {
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_INVALID_NICKNAME
        }.to_line());
        return;
    }

    if (moderator != client.nickname) {
        client.send_line(common::ERR{
            common::CODE_403_FORBIDDEN,
            common::ERR_PERMISSION_DENIED
        }.to_line());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        rooms_.mute_user(room_name, nick, time);
    }

    client.send_line(common::OK{
        common::CODE_200_OK,
        "Muted " + room_name + " " + nick + " " + std::to_string(time)
    }.to_line());
}
void Dispatcher::handle_kick(ClientSession& client,const std::vector<std::string>& args){
    if(args.size() != 2){
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
        return;
    }
        
    std::string room_name = args[0];
    std::string nick = args[1];
    std::string moderator;
    bool has_room = false;
    bool is_target_exist = false;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        has_room = rooms_.has_room(room_name);
        if(has_room){
            moderator = rooms_.get_room(room_name)->moderator_nick;
            is_target_exist = rooms_.is_user_in_room(room_name,nick);
        }
    }
    if(!has_room){
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_INVALID_ROOM_NAME
        }.to_line());
        return;
    }
    if(!is_target_exist){
        client.send_line(common::ERR{
            common::CODE_404_NOT_FOUND,
            common::ERR_MSG_INVALID_NICKNAME
        }.to_line());
        return;
    }
    
    if(moderator != client.nickname){
        client.send_line(common::ERR{
            common::CODE_403_FORBIDDEN,
            common::ERR_PERMISSION_DENIED
        }.to_line());
        return;
    }
    UserRegistry::SessionPtr target_session = users_.find_user(nick);

    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        rooms_.remove_user_from_room(room_name,nick);
    }
    
    if (target_session) {
        target_session->send_line(common::OK{
            common::CODE_200_OK,
            common::OK_KICKER_PREFIX+ room_name
        }.to_line());

        target_session->quit_room(room_name);
    }
    
    std::string leave_event =
    "EVENT LEAVE " + room_name + " " +
    nick + " " + common::timestamp();

    broadcast_event_to_room(room_name, leave_event);
    
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_KICKER_PREFIX+ room_name + " " + nick
    }.to_line());
    

}
void Dispatcher::handle_list(ClientSession& client,
                             const std::vector<std::string>& args) {
    if (args.size() == 1 && args[0] == "ROOMS") {
        std::string list;
        {
            std::lock_guard<std::mutex> lock(rooms_mx_);
            list = rooms_.get_room_list();
        }

        client.send_line(common::OK{
            common::CODE_200_OK,
            common::OK_MSG_ROOMS_PREFIX + list
        }.to_line());
    } else if (args.size() == 2 && args[0] == "USERS" && args[1] != "ONLINE") {
        std::string room_name = args[1];

        bool has_room = false;
        std::string members;
        {
            std::lock_guard<std::mutex> lock(rooms_mx_);
            has_room = rooms_.has_room(room_name);
            if (has_room) {
                members = rooms_.get_members_list(room_name);
            }
        }

        if (!has_room) {
            client.send_line(common::ERR{
                common::CODE_404_NOT_FOUND,
                common::ERR_MSG_NOT_FOUND
            }.to_line());
            return;
        }

        client.send_line(common::OK{
            common::CODE_200_OK,
            common::OK_MSG_USERS_PREFIX + room_name + " " + members
        }.to_line());
    } else if(args.size() == 2 && args[0] == "USERS" && args[1] == "ONLINE"){
        std::vector<std::string> nicks = users_.nicknames();
        std::string nicks_string;
        for(const std::string& nick : nicks){
            nicks_string += nick;
            nicks_string += ' ';
        }
        if(!nicks_string.empty()) nicks_string.pop_back();

        client.send_line(common::OK{
            common::CODE_200_OK,
            common::OK_ONLINE_PREFIX + nicks_string
        }.to_line());
        
    }else {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
    }
}

void Dispatcher::handle_ping(ClientSession& client) {
    client.send_line("PONG");
}
void Dispatcher::handle_pong(ClientSession& client) {

    client.mark_pong_received();
}

void Dispatcher::broadcast_event_to_room(
    const std::string& room_name,
    const std::string& event_line) {

    std::unordered_set<std::string> members_list;
    {
        std::lock_guard<std::mutex> lock(rooms_mx_);
        Room* room = rooms_.get_room(room_name);
        if (!room) {
            return;
        }
        members_list = room->members;
    }

    for (const auto& member_nick : members_list) {
        auto client_member_ptr = users_.find_user(member_nick);
        if (!client_member_ptr) {
            continue;
        }

        client_member_ptr->send_line(event_line);
    }
}
void Dispatcher::handle_status(ClientSession& client,const std::vector<std::string>& args){
    if (args.size() != 1 ||
        (args[0] != "ONLINE" && args[0] != "AWAY" && args[0] != "BUSY")) {
        client.send_line(common::ERR{
            common::CODE_400_BAD_REQUEST,
            common::ERR_MSG_BAD_REQUEST
        }.to_line());
        return;
    }
    if(client.authenticated == false){//
        client.send_line(common::ERR{
            common::CODE_401_UNAUTHORIZED,
            common::ERR_MSG_NOT_LOGGED_IN
        }.to_line());
        return;
    }
    client.status = args[0];
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_STATUS_SET_PREFIX + args[0]
    }.to_line());

    std::string event_message =
        "EVENT STATUS " + client.nickname + " " + common::timestamp() + " " +args[0];
    std::unordered_set<std::string> joined_rooms = client.get_joined_rooms();
    for(const std::string& room : joined_rooms){
        broadcast_event_to_room(room, event_message);
    }
}
void Dispatcher::handle_whoami(ClientSession& client){
    if(client.authenticated == false) {
        client.send_line(common::ERR{
            common::CODE_401_UNAUTHORIZED,
            common::ERR_MSG_NOT_LOGGED_IN
        }.to_line());
        return;
    }
    if(client.status.empty()){
        client.send_line(common::OK{
            common::CODE_200_OK,
            common::OK_UNKNOWN_STATUS + " "+client.nickname +  " "+common::timestamp()
        }.to_line());
        return;
    }
    client.send_line(common::OK{
        common::CODE_200_OK,
        common::OK_YOU_ARE_PREFIX + client.nickname + " "+ client.status + " "+common::timestamp()
    }.to_line());
}
}