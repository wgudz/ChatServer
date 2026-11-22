#include "server/client_session.hpp"

#include "common/net_utils.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <iostream>
#include <vector>
namespace server {

bool ClientSession::recv_line(std::string& out_line, bool& conn_closed) {
  constexpr size_t MAX_LINE_LEN = 1024;
  common::RecvStatus st = common::recv_line_fd(fd,inbuf,out_line,conn_closed, MAX_LINE_LEN);
  switch(st){
    case common::RecvStatus::Ok:
        {
        conn_closed = false;
        touch();
        auto now = std::chrono::steady_clock::now();
        if (now - last_reset >= std::chrono::seconds(1)  ) {
            last_reset = now;
            cmd_count = 0;
        }
        ++cmd_count;
        return true; 
        }
    case common::RecvStatus::Closed:
        conn_closed = true;
        return false;
    case common::RecvStatus::TooLong:
        out_line.clear();
        send_line("ERR 413 Line too long.");
        conn_closed = false;
        return true;
    case common::RecvStatus::Error:
    default:
        conn_closed = true;
        return false;
  }
    return false;
}
void server::ClientSession::force_close() {
    if (fd >= 0) {
        ::shutdown(fd, SHUT_RDWR);
    }
}
bool ClientSession::send_line(const std::string& line) const {
  std::lock_guard<std::mutex> lock(send_mx);
  return common::send_line_fd(fd,line);
}
bool ClientSession::is_in_room(const std::string& room) const {
    std::lock_guard<std::mutex> lk(rooms_mx_);
  return joined_rooms.count(room) > 0;
}

void ClientSession::join_room(const std::string& room) {
    std::lock_guard<std::mutex> lk(rooms_mx_);
  joined_rooms.insert(room);
}
void ClientSession::quit_room(const std::string& room) {
    std::lock_guard<std::mutex> lk(rooms_mx_);
    joined_rooms.erase(room);
}

void ClientSession::leave_room(const std::string& room) {
    std::lock_guard<std::mutex> lk(rooms_mx_);
  joined_rooms.erase(room);
}
void ClientSession::mark_ping_sent(std::chrono::steady_clock::time_point now) {
    {
        std::lock_guard<std::mutex> lk(ping_mx_);
        last_ping_time_ = now;
    }
    waiting_for_pong_.store(true, std::memory_order_release);
}

void ClientSession::mark_pong_received() {
    waiting_for_pong_.store(false, std::memory_order_release);
    std::lock_guard<std::mutex> lk(ping_mx_);
    last_ping_time_ = std::chrono::steady_clock::now();
}

bool ClientSession::ping_timed_out(std::chrono::steady_clock::time_point now,
                                   std::chrono::seconds timeout) const {
    if (!waiting_for_pong_.load(std::memory_order_acquire)) {
        return false;
    }
    std::chrono::steady_clock::time_point last;
    {
        std::lock_guard<std::mutex> lk(ping_mx_);
        last = last_ping_time_;
    }
    return (now - last) > timeout;
}
void ClientSession::touch() {
    last_activity_ = std::chrono::steady_clock::now();
}

} // namespace server