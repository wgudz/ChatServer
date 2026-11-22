#pragma once
#include <string>
namespace common {
struct OK{
  int code;
  std::string msg;
  std::string to_line() const;
};
struct ERR{
  int code;
  std::string msg;
  std::string to_line() const;
};
inline constexpr int CODE_100_READY             = 100;
inline constexpr int CODE_200_OK                = 200;
inline constexpr int CODE_400_BAD_REQUEST       = 400;
inline constexpr int CODE_401_UNAUTHORIZED      = 401;
inline constexpr int CODE_403_FORBIDDEN         = 403;
inline constexpr int CODE_404_NOT_FOUND         = 404;
inline constexpr int CODE_408_PING_TIMEOUT      = 408;
inline constexpr int CODE_409_CONFLICT          = 409;
inline constexpr int CODE_413_LINE_TOO_LONG     = 413;
inline constexpr int CODE_429_TOO_MANY_REQUESTS = 429;
inline constexpr int CODE_500_INTERNAL_ERROR    = 500;
  
  
inline const std::string HELLO = "HELLO 1.0";
inline const std::string WELCOME = "Welcome ChatServer 1.0";
inline const std::string PING    = "PING";
inline const std::string PONG    = "PONG";
inline const std::string BYE     = "BYE";
inline const std::string UNSUPPORTED_PROTOCOL = "Unsupported protocol";

inline const std::string OK_MSG_READY              = "Ready";
inline const std::string OK_MSG_NICK_SET           = "Nick set";
inline const std::string OK_MSG_JOINED_PREFIX      = "Joined ";
inline const std::string OK_MSG_LEFT_PREFIX        = "Left ";
inline const std::string OK_MSG_DELIVERED          = "Delivered";
inline const std::string OK_MSG_ROOMS_PREFIX       = "ROOMS ";
inline const std::string OK_MSG_USERS_PREFIX       = "USERS ";
inline const std::string OK_MSG_ROOM_CREATED       = "Room has been created";
inline const std::string OK_HANDLED_PING           = "You handled PING";
inline const std::string OK_ONLINE_PREFIX = "Online ";
inline const std::string OK_STATUS_SET_PREFIX = "Status set ";
inline const std::string OK_YOU_ARE_PREFIX  = "You are ";
inline const std::string OK_UNKNOWN_STATUS =  " Your status is unknown";
inline const std::string OK_KICKER_PREFIX =  "Kicked ";
inline const std::string ERR_PING_TIMEOUT       = "No PONG received, disconnecting";
inline const std::string ERR_MSG_BAD_REQUEST       = "Bad request";
inline const std::string ERR_MSG_SYNTAX_ERROR      = "Syntax error";
inline const std::string ERR_MSG_INVALID_NICKNAME  = "Invalid nickname";
inline const std::string ERR_MSG_INVALID_ROOM_NAME = "Invalid room name";
inline const std::string ERR_MSG_NOT_LOGGED_IN     = "Not logged in";
inline const std::string ERR_MSG_NOT_IN_ROOM       = "Not in room";
inline const std::string ERR_MSG_NOT_FOUND         = "Not found";
inline const std::string ERR_MSG_NICK_IN_USE       = "Nick in use";
inline const std::string ERR_MSG_LINE_TOO_LONG     = "Line too long";
inline const std::string ERR_MSG_TOO_MANY_REQUESTS = "Too Many Requests";
inline const std::string ERR_MSG_INTERNAL_ERROR    = "Internal Server Error";
inline const std::string ERR_PERMISSION_DENIED   = "Permisson denied";
inline const std::string MUTED_PREFIX   = "Muted until ";
inline const std::string EVENT_LEAVE_PREFIX        = "Event leave ";
inline const std::string ALREADY_IN_ROOM           = "Already in room";
inline const std::string MSG_BYE                   = "Bye";

//colors
constexpr const char* reset = "\x1b[0m";
constexpr const char* red   = "\x1b[31m";
constexpr const char* green = "\x1b[32m";
constexpr const char* yellow= "\x1b[33m";
constexpr const char* blue  = "\x1b[34m";
constexpr const char* cyan  = "\x1b[36m";
constexpr const char* bold  = "\x1b[1m";


}