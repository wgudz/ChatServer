#pragma once
#include <string>

namespace common {

struct Config {
    std::string host;
    int port = 0;
};

Config parse_args(int argc, char** argv);
  
}
