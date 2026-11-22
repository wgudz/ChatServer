#include "client/receiver.hpp"
#include "client/history.hpp"
#include "common/protocol.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
namespace client {

std::thread start_receiver(Connector& connector, std::atomic<bool>& running) {
    return std::thread([&connector, &running]() {
        std::string line;
        bool closed = false;
        common::OK ok{common::CODE_200_OK, common::MSG_BYE};
        
        while (running && !closed) {
            if (!connector.recv_line(line, closed)) {
                if (!closed) {
                    std::cerr << "Połączenie przerwane\n";
                }
                break;
            }
            if (line.empty()) {
                continue;
            }
            if (line == "PING") {
                connector.send_line("PONG");
                continue;
            }

            client::history_append(line);
            std::cout << paint(line) << std::endl;
            
            if( line == ok.to_line() ){
                break;
            }
        }
        running = false;
    });
}
std::string paint(const std::string& line) {
    if (line.rfind("ERR ", 0) == 0)
        return std::string("\x1b[31m") + line + "\x1b[0m";
    if (line.rfind("OK ", 0) == 0)
        return std::string("\x1b[32m") + line + "\x1b[0m";
    if (line.rfind("WELCOME ", 0) == 0)
        return std::string("\x1b[36m") + line + "\x1b[0m";
    if (line.rfind("EVENT JOIN ", 0) == 0 ||
        line.rfind("EVENT LEAVE ", 0) == 0)
        return std::string("\x1b[33m") + line + "\x1b[0m";
    if (line.rfind("EVENT PRIV ", 0) == 0)
        return std::string("\x1b[35m") + line + "\x1b[0m";
    if (line == "PING" || line == "PONG")
        return std::string("\x1b[35m") + line + "\x1b[0m";
    if (line.rfind("EVENT STATUS ", 0) == 0)
        return std::string("\x1b[33m") + line + "\x1b[0m";
    return line;

}
} // namespace client
