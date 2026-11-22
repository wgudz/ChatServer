#include "common/protocol.hpp"

namespace common {

std::string OK::to_line() const {
  return "OK " + std::to_string(code) + " " + msg;
}

std::string ERR::to_line() const {
  return "ERR " + std::to_string(code) + " " + msg;
}

  
}