#include "server/logger.hpp"

namespace server {

bool Logger::open(const std::string& path) {
    file_.open(path, std::ios::app);
    return file_.is_open();
}

void Logger::log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(mx);
    if (file_.is_open())
        file_ << msg << std::endl;
}

} // namespace server