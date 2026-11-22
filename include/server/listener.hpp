#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

namespace server {


class Listener {
    int sockfd = -1;
public:
    bool init(const std::string& host,int port);
    int get_socket() const;
};


} // namespace server
