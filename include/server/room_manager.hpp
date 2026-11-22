#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <chrono>

namespace server {

struct Room {
    std::string name;
    std::unordered_set<std::string> members;
    std::deque<std::string> history;
    std::string moderator_nick;
    std::unordered_map<
        std::string,
        std::chrono::steady_clock::time_point
    > mutes;
};

class RoomManager {
public:
    RoomManager(std::string file_path)
        : file_path_(std::move(file_path))
    {}
    bool create_room(const std::string& name);
    bool has_room(const std::string& name) const;
    Room* get_room(const std::string& name);

    bool load_from_file();
    bool save_to_file() const;

    bool add_user_to_room(const std::string& room_name,
      const std::string& nickname);
    bool remove_user_from_room(const std::string& room_name,
    const std::string& nickname);
    bool is_user_in_room(const std::string& room_name,
     const std::string& nickname) const;
    bool room_empty(const std::string& name) const;
    std::string get_room_list() const;
    std::string get_members_list(const std::string& room_name) const;
    std::unordered_set<std::string> members(const std::string& room_name) const;
    void delete_room(const std::string& name);
    
    
    void ensure_default_general();
    void add_message(const std::string& room_name,const std::string& msg);
    std::string get_history(const std::string& room_name);
    void mute_user(const std::string& room_name,
    const std::string& nick,
    int seconds);
    void append_history_to_disk(const std::string& room_name);
private:
    std::unordered_map<std::string, Room> rooms_;
    std::string file_path_;
    std::string default_room = "#general";
    std::size_t history_limit = 30;
};

} // namespace server
