#include "common/net_utils.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <algorithm>

namespace common {
bool write_all_fd(int fd, const char* data, size_t len) {
    const char* p = data; size_t left = len;
    while (left) {
        ssize_t n = ::send(fd, p, left, MSG_NOSIGNAL);
        if (n < 0) { if (errno == EINTR) continue; return false; }
        left -= (size_t)n; p += n;
    }
    return true;
}

bool send_line_fd(int fd, const std::string& line) {
    if (!write_all_fd(fd, line.data(), line.size())) return false;
    const char nl = '\n';
    return write_all_fd(fd, &nl, 1);
}

RecvStatus recv_line_fd(int fd,
                        std::string& inbuf,
                        std::string& out_line,
                        bool& closed,
                        size_t max_line_len)
{
    closed = false;

    if (auto pos = inbuf.find('\n'); pos != std::string::npos) {
        out_line = inbuf.substr(0, pos);
        inbuf.erase(0, pos + 1);
        return RecvStatus::Ok;
    }

    char buf[4096];

    for (;;) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);

        if (n == 0) {
            closed = true;
            return RecvStatus::Closed;
        }

        if (n < 0) {
            if (errno == EINTR)
                continue;
            closed = true;
            return RecvStatus::Error;
        }

        inbuf.append(buf, buf + n);

        if (max_line_len > 0 && inbuf.size() > max_line_len) {
            inbuf.clear();
            out_line.clear();
            closed = false;

            char buf2[4096];

            for (;;) {
                ssize_t m = ::recv(fd, buf2, sizeof(buf2), 0);

                if (m == 0) {
                    closed = true;
                    return RecvStatus::Closed;
                }
                if (m < 0) {
                    if (errno == EINTR) continue;
                    closed = true;
                    return RecvStatus::Error;
                }

                auto it = std::find(buf2, buf2 + m, '\n');
                if (it != buf2 + m) {
                    inbuf.append(it + 1, buf2 + m);
                    return RecvStatus::TooLong;
                }
            }
        }

        if (auto pos = inbuf.find('\n'); pos != std::string::npos) {
            out_line = inbuf.substr(0, pos);
            inbuf.erase(0, pos + 1);
            return RecvStatus::Ok;
        }
    }
}
    
}