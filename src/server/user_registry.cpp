#include "server/user_registry.hpp"

namespace server {

bool UserRegistry::add_user(const std::string& nickname, SessionPtr session) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto [it, inserted] = users_.emplace(nickname, std::move(session));
  if (!inserted) {
        return false;
  }
  return true;
}
void UserRegistry::remove_user(const std::string& nickname) {
  std::lock_guard<std::mutex> lock(mtx_);
  users_.erase(nickname);
}

UserRegistry::SessionPtr UserRegistry::find_user(const std::string& nickname) const {
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = users_.find(nickname);
  if (it == users_.end()) {
      return nullptr;
  }
  return it->second;
}

bool UserRegistry::has_user(const std::string& nickname) const {
  std::lock_guard<std::mutex> lock(mtx_);
  return users_.find(nickname) != users_.end();
}
std::vector<UserRegistry::SessionPtr> UserRegistry::snapshot_sessions() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<SessionPtr> result;
    result.reserve(users_.size());
    for (const auto& [nick, sess] : users_) {
        result.push_back(sess);
    }
    return result;
}
std::vector<std::string> UserRegistry::nicknames() const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<std::string> nicks;
    for(const auto& [nick,_]  : users_){
        nicks.push_back(nick);
    }
    return nicks;
}
}