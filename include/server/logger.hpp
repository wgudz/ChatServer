#pragma once
#include <fstream>
#include <string>
#include <mutex>

namespace server {

class Logger {
public:
    bool open(const std::string& path);
    void log(const std::string& msg);

private:
    std::ofstream file_;
    std::mutex mx;
};

} // namespace server