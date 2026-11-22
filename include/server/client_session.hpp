#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <atomic>

namespace server {

class ClientSession {
public:
  explicit ClientSession(int fd_) : fd(fd_) {}

  void force_close();
  
  bool recv_line(std::string& out_line, bool& conn_closed);
  bool send_line(const std::string& line) const;

  bool is_in_room(const std::string& room) const;
  void join_room(const std::string& room);
  void quit_room(const std::string& room);
  void leave_room(const std::string& room);
  std::unordered_set<std::string> get_joined_rooms() const {
    std::lock_guard<std::mutex> lock(rooms_mx_);
    return joined_rooms;
  }
  
  bool authenticated{false};
  std::string nickname;
  std::string status;

  int commands_last_second() const { return cmd_count; }


  void touch();

  void mark_ping_sent(std::chrono::steady_clock::time_point now);
  void mark_pong_received();
  bool ping_timed_out(std::chrono::steady_clock::time_point now,
                      std::chrono::seconds timeout) const;
  bool is_waiting_for_pong(){return waiting_for_pong_.load();}
  std::chrono::steady_clock::time_point last_ping_time() const {
      std::lock_guard<std::mutex> lk(ping_mx_);
      return last_ping_time_;
  }
private:
  int fd{-1};
  std::string inbuf;
  std::unordered_set<std::string> joined_rooms;
  mutable std::mutex send_mx;
  int cmd_count = 0;
  
  std::atomic<bool> waiting_for_pong_{false};
  std::chrono::steady_clock::time_point last_reset{};
  std::chrono::steady_clock::time_point last_activity_{};
  std::chrono::steady_clock::time_point last_ping_time_{};

  mutable std::mutex ping_mx_;
  mutable std::mutex rooms_mx_;

};

} // namespace server