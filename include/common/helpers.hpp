#pragma once

#include <vector>
#include <string>

namespace common {
  
bool starts_with(const std::string& s, const std::string& pfx);

std::string get_message(const std::vector<std::string>& args);
}
