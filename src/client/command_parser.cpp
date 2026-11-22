#include "client/command_parser.hpp"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace client {

std::string to_protocol(const std::string& input){
  std::istringstream iss(input);
  std::string cmd;
  if (!(iss >> cmd)) return {};

  std::string cmd_lower = cmd;
  std::transform(cmd_lower.begin(), cmd_lower.end(), cmd_lower.begin(),
                 [](unsigned char c){ return std::tolower(c); });

  std::string rest;
  std::getline(iss, rest);
  if (!rest.empty() && rest[0] == ' ')
      rest.erase(0, 1);

  std::string cmd_upper = cmd;
  std::transform(cmd_upper.begin(), cmd_upper.end(), cmd_upper.begin(),
                 [](unsigned char c){ return std::toupper(c); });

  if (cmd_lower == "quit")
      return "QUIT";
    if (cmd_lower == "/whoami")
        return "WHOAMI";
    if (cmd_lower == "list" && rest == "users online")
        return "LIST USERS ONLINE";
    if (cmd_lower == "status") {
        std::string opt = rest;
        std::transform(opt.begin(), opt.end(), opt.begin(),
                       [](unsigned char c){ return std::toupper(c); });
        return "STATUS " + opt;
    }
  if (cmd_lower == "list") {
      if (rest.empty())
          return "LIST";

      std::istringstream iss2(rest);
      std::string sub;
      if (!(iss2 >> sub))
          return "LIST";

      std::string rest2;
      std::getline(iss2, rest2);
      if (!rest2.empty() && rest2[0] == ' ')
          rest2.erase(0, 1);

      std::transform(sub.begin(), sub.end(), sub.begin(),
                     [](unsigned char c){ return std::toupper(c); });

      if (rest2.empty())
          return "LIST " + sub;
      return "LIST " + sub + " " + rest2;
  }
    
  if (rest.empty())
      return cmd_upper;
  return cmd_upper + " " + rest;
}
  
}


