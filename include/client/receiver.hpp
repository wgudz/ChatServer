#pragma once
#include <atomic>
#include <thread>
#include <string>
#include "client/connector.hpp"

namespace client {

std::thread start_receiver(Connector& connector, std::atomic<bool>& running);

std::string paint(const std::string& line);
} // namespace client