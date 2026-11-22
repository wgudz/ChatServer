#include "common/time_utils.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace common {

std::string timestamp() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);

  std::tm tm{};
  gmtime_r(&t, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
  
}
  
}
