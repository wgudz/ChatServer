#pragma once
#include <string>
namespace common {

enum class RecvStatus {
  Ok,
  Closed,
  TooLong,
  Error
};
  
bool write_all_fd(int fd,const char* data,size_t len);
bool send_line_fd(int fd,const std::string& line);
RecvStatus recv_line_fd(int fd,
  std::string& inbuf,
  std::string& out_line,
  bool& closed,
  size_t max_line_len);
}