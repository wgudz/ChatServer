#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "server/client_session.hpp"

namespace server {

class UserRegistry {
public:
  using SessionPtr = std::shared_ptr<ClientSession>;
  bool add_user(const std::string& nickname, SessionPtr session);
  void remove_user(const std::string& nickname);
  SessionPtr find_user(const std::string& nickname) const;
  bool has_user(const std::string& nickname) const;
  std::vector<SessionPtr> snapshot_sessions() const;
  std::vector<std::string> nicknames() const;
private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, SessionPtr> users_;
};
  
}