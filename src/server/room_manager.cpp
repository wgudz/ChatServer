#include "server/room_manager.hpp"
#include <fstream>
#include <filesystem>

namespace server {
void RoomManager::delete_room(const std::string& name){
    if(name == default_room) return;
    rooms_.erase(name);
}
bool RoomManager::create_room(const std::string& name) {
    return rooms_.emplace(name, Room{ name, {} }).second;
}

bool RoomManager::has_room(const std::string& name) const {
    return rooms_.find(name) != rooms_.end();
}

Room* RoomManager::get_room(const std::string& name) {
    auto it = rooms_.find(name);
    return it == rooms_.end() ? nullptr : &it->second;
}

bool RoomManager::load_from_file() {
    rooms_.clear();
    std::ifstream in(file_path_);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty())
            rooms_.emplace(line, Room{ line, {} });
    }
    return true;
}

bool RoomManager::save_to_file() const {
    const auto parent = std::filesystem::path(file_path_).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);

    std::ofstream out(file_path_, std::ios::trunc);
    if (!out.is_open()) return false;

    for (const auto& kv : rooms_)
        out << kv.first << '\n';

    return true;
}

void RoomManager::ensure_default_general() {
    if (!has_room("#general"))
        create_room("#general");
}

bool RoomManager::add_user_to_room(const std::string& room_name,
                                   const std::string& nickname)
{
    auto it = rooms_.find(room_name);
    if( it == rooms_.end() ) return false;

    it -> second.members.insert(nickname);
    return true;
}
bool RoomManager::remove_user_from_room(const std::string& room_name,
const std::string& nickname){
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) return false;

    return it->second.members.erase(nickname);
}
bool RoomManager::is_user_in_room(const std::string& room_name,
                                  const std::string& nickname) const
{
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) return false;

    return it->second.members.count(nickname) > 0;
}
bool RoomManager::room_empty(const std::string& name) const {
    auto it = rooms_.find(name);
    if (it == rooms_.end()) return true; // return true when room does not exist
    return it->second.members.empty();
}
std::string RoomManager::get_room_list() const {
    std::string result;
    for (const auto& [name, room] : rooms_) {
        result += name + " ";
    }
    if (!result.empty())
        result.pop_back();

    return result;
}
std::string RoomManager::get_members_list(const std::string& room_name) const{
    auto it = rooms_.find(room_name);
    if (it == rooms_.end()) {
        return "";
    }

    const auto& members = it->second.members;

    std::string result;
    for (const auto& m : members) {
        if (!result.empty()) {
            result += " ";
        }
        result += m;
    }

    return result;
}
std::unordered_set<std::string> RoomManager::members(const std::string& room_name) const{
    auto it = rooms_.find(room_name);
    if(it == rooms_.end()) {
        return {};
    }
    return it->second.members;
}
void RoomManager::add_message(const std::string& room_name,const std::string& msg){
    Room* room = get_room(room_name);
    if (!room)
        return;

    room->history.push_back(msg);
    if (room->history.size() > history_limit)
        room->history.pop_front();
}
std::string RoomManager::get_history(const std::string& room_name){
    Room* room = get_room(room_name);
    if (!room)
        return "";
    std::string out;
    for (const std::string& msg : room->history) {
        out += msg;
        out += '\n';
    }
    if (!out.empty()) out.pop_back();
    return out;
}
void RoomManager::mute_user(const std::string& room_name,
const std::string& nick,
int seconds)
{
    if (seconds <= 0)
        return;

    Room* room = get_room(room_name);
    if (!room)
        return;

    room->mutes[nick] =
        std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
}
void RoomManager::append_history_to_disk(const std::string& room_name){
    Room* room = nullptr;
    room = get_room(room_name);
    if (!room or room->history.empty())
        return;
    namespace fs = std::filesystem;
    fs::path base = fs::path(file_path_).parent_path();
    fs::path history_dir = base / "history";
    fs::create_directories(history_dir);
    std::string safe_name;
    safe_name.reserve(room -> name.size());
    for (unsigned char c : room -> name) {
        if (std::isalnum(c) || c == '_' || c == '-')
            safe_name.push_back(static_cast<char>(c));
        else
            safe_name.push_back('_');
    }

    fs::path file_path = history_dir / (safe_name + ".log");

    std::ofstream out(file_path, std::ios::app);
    if (!out) return;
    if (!room -> history.empty()) {
        out << room -> history.back() << '\n';
        out.flush();
    }
}
} // namespace server