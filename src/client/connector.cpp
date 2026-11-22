#include "client/connector.hpp"
#include "common/net_utils.hpp"
#include "common/protocol.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <iostream>

namespace client {

Connector::~Connector() { disconnect(); }
  
void Connector::disconnect() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
        inbuf_.clear();
    }
}

bool Connector::connect_to(const std::string& host, int port) {
    disconnect();

    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    std::snprintf(port_str, sizeof(port_str), "%d", port);

    addrinfo* res = nullptr;
    int rc = ::getaddrinfo(host.c_str(), port_str, &hints, &res);
    if (rc != 0) return false;

    int fd = -1;
    for (addrinfo* ai = res; ai; ai = ai->ai_next) {
        fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;

        if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            sockfd_ = fd;
            break;
        }
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(res);
    return sockfd_ >= 0;
}

bool Connector::send_line(const std::string& line) const {
    if (sockfd_ < 0) return false;
    return common::send_line_fd(sockfd_, line);
}

bool Connector::recv_line(std::string& out_line, bool& conn_closed) {
    if (sockfd_ < 0) { conn_closed = true; return false; }
    
  common::RecvStatus st = common::recv_line_fd(sockfd_,inbuf_,out_line,conn_closed,0);
  switch(st){
    case common::RecvStatus::Ok:
        conn_closed = false;
        return true;

    case common::RecvStatus::Closed:
        conn_closed = true;
        return false;
    case common::RecvStatus::TooLong:
        conn_closed = true;
        return false;
    case common::RecvStatus::Error:
    default:
        conn_closed = true;
        return false;
  }
    return true;
}

bool Connector::handshake() {
    if (!send_line(common::HELLO)) return false;

    std::string line;
    bool closed = false;
    if (!recv_line(line, closed)) return false;

    if (line.rfind("Welcome ", 0) != 0) return false;
    if (line.size() < 3 || line.substr(line.size() - 3) != "1.0") return false;
    std::cout << line;
    return true;
}

} // namespace client

