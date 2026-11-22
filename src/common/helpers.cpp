#include "common/helpers.hpp"

namespace common {
  
bool starts_with(const std::string& s, const std::string& pfx){ return s.rfind(pfx,0)==0; }

  std::string get_message(const std::vector<std::string>& args) {
      if (args.size() < 2) return "";

      std::string msg;
      for (size_t i = 1; i < args.size(); ++i) {
          if (!msg.empty()) msg += ' ';
          msg += args[i];
      }
      return msg;
  }
  
}