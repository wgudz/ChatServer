#include "server/listener.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <string>

namespace server {

bool Listener::init(const std::string& host, int port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;     // IPv4 lub IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // dla bind()

    addrinfo* result;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        perror("getaddrinfo");
        return false;
    }

    sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(result);
        return false;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(sockfd, result->ai_addr, result->ai_addrlen) < 0) {
        perror("bind");
        close(sockfd);
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);

    if (listen(sockfd, 1000) < 0) {
        perror("listen");
        close(sockfd);
        return false;
    }

    std::cout << "Nasłuchiwanie na " << host << ":" << port << "...\n";
    return true;
}


int Listener::get_socket() const {
    return sockfd;
}

} // namespace server