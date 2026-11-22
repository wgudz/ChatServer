#include "client/history.hpp"

#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace client {

namespace {
    const char* HISTORY_FILE = "chat_history.txt";

    std::string current_timestamp() {
        using clock = std::chrono::system_clock;
        auto now = clock::now();
        std::time_t t = clock::to_time_t(now);

        std::tm tm{};
    #ifdef _WIN32
        localtime_s(&tm, &t);
    #else
        localtime_r(&t, &tm);
    #endif

        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]";
        return oss.str();
    }
}

void history_append(const std::string& line) {
    std::ofstream out(HISTORY_FILE, std::ios::app);
    if (!out) {
        return;
    }

    out << current_timestamp() << " " << line << "\n";
}

}
