#pragma once
#include <string>

namespace client{
  
class Connector {
public:
  Connector() = default;
  ~Connector();

  void disconnect();
  
  bool connect_to(const std::string& host, int port);
  bool handshake();
  bool send_line(const std::string& line) const;
  bool recv_line(std::string& out_line,bool& conn_closed);
  int fd() const { return sockfd_;}
  bool ok() const { return sockfd_ >= 0;}
private:
  int sockfd_{-1};
  std::string inbuf_;
};
  
}

