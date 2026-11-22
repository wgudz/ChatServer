#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <cerrno>
#include <cstring>


static constexpr const char* HOST = "127.0.0.1";
static constexpr int PORT = 5555;
static const std::string HELLO   = "HELLO 1.0";
static const std::string WELCOME = "Welcome ChatServer 1.0";

static constexpr int IO_TIMEOUT_MS = 1500;

static void set_timeout(int fd, int ms) {
    timeval tv{};
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static void send_line(int fd, const std::string& s) {
    std::string x = s + "\n";
    (void)send(fd, x.c_str(), x.size(), 0);
}

static bool recv_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n == 0) return false; // closed
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) return false; // timeout
            std::cerr << "[recv error] " << strerror(errno) << "\n";
            return false;
        }
        if (c == '\n') return true;
        if (c != '\r') out.push_back(c);
    }
}

static int connect_to() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "[socket] " << strerror(errno) << "\n";
        return -1;
    }

    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &a.sin_addr);

    if (connect(fd, (sockaddr*)&a, sizeof(a)) != 0) {
        std::cerr << "[connect] " << strerror(errno) << "\n";
        close(fd);
        return -1;
    }

    set_timeout(fd, IO_TIMEOUT_MS);
    return fd;
}


static void drain(int fd, const char* tag, int max_lines = 200) {
    std::string line;
    int n = 0;
    while (n < max_lines && recv_line(fd, line)) {
        std::cout << "[" << tag << " <=] " << line << "\n";
        ++n;
    }
    std::cout << "[" << tag << "] drained " << n << " line(s)\n" << std::endl;
}

static bool do_handshake(int fd, const char* tag) {

    std::cout << "[" << tag << " =>] " << HELLO << "\n";
    send_line(fd, HELLO);


    std::string line;
    for (int i = 0; i < 20; ++i) {
        if (!recv_line(fd, line)) break;
        std::cout << "[" << tag << " <=] " << line << "\n";
        if (line == WELCOME) {
            std::cout << "[" << tag << "] handshake OK\n" << std::endl;
            drain(fd, tag);
            return true;
        }
    }

    std::cout << "[" << tag << "] handshake FAIL (no WELCOME)\n" << std::endl;
    drain(fd, tag);
    return false;
}

int main() {
    std::cout << "[TEST] starting server...\n" << std::endl;

    pid_t pid = fork();
    if (pid == 0) {
        execl("./build/server", "./build/server",
              "--host", HOST,
              "--port", "5555",
              nullptr);
        _exit(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    int a = connect_to(); // Dede
    int b = connect_to(); // Ala
    if (a < 0 || b < 0) {
        std::cerr << "[TEST] connect failed\n";
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        return 2;
    }

    bool ok = true;


    if (!do_handshake(a, "A")) ok = false;
    if (!do_handshake(b, "B")) ok = false;

    if (ok) {
        std::cout << "[TEST] sending commands...\n" << std::endl;

        // Dede
        send_line(a, "NICK Dede1234");
        drain(a, "A");

        send_line(a, "JOIN #testroom");
        drain(a, "A");

        // Ala
        send_line(b, "NICK Ala1234");
        drain(b, "B");

        send_line(b, "JOIN #testroom");
        drain(b, "B");

        // EVENT JOIN Ala → Dede
        drain(a, "A");

        // MSG
        send_line(a, "MSG #testroom co tam ala?");
        drain(a, "A");

        std::cout << "[TEST] waiting for message on B...\n" << std::endl;

        bool got = false;
        std::string line;

        for (int i = 0; i < 400; ++i) {
            if (!recv_line(b, line)) break;
            std::cout << "[B <=] " << line << "\n";
            if (line.find("co tam ala?") != std::string::npos) {
                got = true;
                break;
            }
        }

        if (got) {
            std::cout << "\n[PASS] B received: 'co tam ala?'\n" << std::endl;
        } else {
            std::cout << "\n[FAIL] B did NOT receive: 'co tam ala?'\n" << std::endl;
            ok = false;
        }
    }

    close(a);
    close(b);
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);

    return ok ? 0 : 1;
}
