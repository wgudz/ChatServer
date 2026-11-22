#include "common/config.hpp"
#include "common/helpers.hpp"
#include <unordered_map>

#include <iostream>
namespace common {

Config parse_args(int argc, char** argv)
{
    std::unordered_map<std::string, std::string> args;

    for (int i = 1; i < argc; i++) {
        std::string k = argv[i];

        if (common::starts_with(k, "--")) {
            std::string value;
            while (i + 1 < argc && !common::starts_with(argv[i + 1], "--")) {
                if (!value.empty()) value += " ";
                value += argv[++i];
            }
            args[k] = value;
        } else {
            args[k] = "";
        }
    }

    Config cfg;

    // HOST
    auto host_it = args.find("--host");
    if (host_it == args.end() || host_it->second.empty()) {
        std::cerr << "missing host\n";
        return cfg;
    }
    cfg.host = host_it->second;

    // PORT
    auto port_it = args.find("--port");
    if (port_it == args.end() || port_it->second.empty()) {
        std::cerr << "missing port\n";
        return cfg;
    }

    try {
        cfg.port = std::stoi(port_it->second);
    } catch (...) {
        std::cerr << "port must be number\n";
        return Config{};
    }

    return cfg;
}

}
