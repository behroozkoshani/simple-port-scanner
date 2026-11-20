#include "scanner.hpp"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace bk {
namespace {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;
int last_error() { return WSAGetLastError(); }
bool in_progress(int error) { return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS; }
bool refused(int error) { return error == WSAECONNREFUSED; }
bool timed_out(int error) { return error == WSAETIMEDOUT; }
void close_socket(Socket socket) { closesocket(socket); }
bool nonblocking(Socket socket) { u_long mode = 1; return ioctlsocket(socket, FIONBIO, &mode) == 0; }
struct NetworkInit {
    NetworkInit() {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) throw std::runtime_error("Winsock initialization failed");
    }
    ~NetworkInit() { WSACleanup(); }
};
#else
using Socket = int;
constexpr Socket invalid_socket = -1;
int last_error() { return errno; }
bool in_progress(int error) { return error == EINPROGRESS || error == EWOULDBLOCK; }
bool refused(int error) { return error == ECONNREFUSED; }
bool timed_out(int error) { return error == ETIMEDOUT; }
void close_socket(Socket socket) { ::close(socket); }
bool nonblocking(Socket socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
}
struct NetworkInit {};
#endif

struct SocketGuard {
    Socket fd = invalid_socket;
    ~SocketGuard() { if (fd != invalid_socket) close_socket(fd); }
};

int number(const std::string& token) {
    if (token.empty()) throw std::invalid_argument("Port cannot be empty");
    int value = 0;
    auto [ptr, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc{} || ptr != token.data() + token.size() || value < 1 || value > 65535)
        throw std::invalid_argument("Invalid TCP port: " + token);
    return value;
}

bool loopback_address(const in_addr& address) {
    const std::uint32_t host = ntohl(address.s_addr);
    return (host >> 24) == 127;
}

sockaddr_in resolve_target(const Options& options) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* answers = nullptr;
    const int status = getaddrinfo(options.target.c_str(), nullptr, &hints, &answers);
    if (status != 0 || !answers) throw std::runtime_error("Unable to resolve IPv4 target: " + options.target);
    const auto resolved = *reinterpret_cast<const sockaddr_in*>(answers->ai_addr);
    freeaddrinfo(answers);
    if (!options.allow_remote && !loopback_address(resolved.sin_addr))
        throw std::invalid_argument("Non-loopback target blocked. Scan only authorized systems; supply --allow-remote to confirm permission.");
    return resolved;
}

Result inspect(sockaddr_in address, int port, int timeout_ms) {
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    SocketGuard guard;
    guard.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (guard.fd == invalid_socket) return {port, State::Error, "socket creation failed"};
    if (!nonblocking(guard.fd)) return {port, State::Error, "nonblocking setup failed"};

    const int result = connect(guard.fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    if (result == 0) return {port, State::Open, "TCP handshake completed"};
    int error = last_error();
    if (refused(error)) return {port, State::Closed, "connection refused"};
    if (!in_progress(error)) return {port, timed_out(error) ? State::Filtered : State::Error, "connection failed (OS error " + std::to_string(error) + ")"};

    fd_set writable;
    fd_set exceptional;
    FD_ZERO(&writable);
    FD_ZERO(&exceptional);
    FD_SET(guard.fd, &writable);
    FD_SET(guard.fd, &exceptional);
    timeval timeout{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
#ifdef _WIN32
    const int selected = select(0, nullptr, &writable, &exceptional, &timeout);
#else
    const int selected = select(guard.fd + 1, nullptr, &writable, &exceptional, &timeout);
#endif
    if (selected == 0) return {port, State::Filtered, "connect timeout (may be filtered)"};
    if (selected < 0) return {port, State::Error, "select failed"};

    int so_error = 0;
#ifdef _WIN32
    int length = sizeof(so_error);
    const int status = getsockopt(guard.fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &length);
#else
    socklen_t length = sizeof(so_error);
    const int status = getsockopt(guard.fd, SOL_SOCKET, SO_ERROR, &so_error, &length);
#endif
    if (status != 0) return {port, State::Error, "socket status unavailable"};
    if (so_error == 0) return {port, State::Open, "TCP handshake completed"};
    if (refused(so_error)) return {port, State::Closed, "connection refused"};
    if (timed_out(so_error)) return {port, State::Filtered, "connect timeout (may be filtered)"};
    return {port, State::Error, "OS error " + std::to_string(so_error)};
}

} // namespace

std::vector<int> parse_ports(const std::string& spec) {
    if (spec.empty()) throw std::invalid_argument("--ports requires a value");
    std::vector<int> ports;
    std::size_t start = 0;
    for (;;) {
        const auto comma = spec.find(',', start);
        const std::string token = spec.substr(start, comma == std::string::npos ? comma : comma - start);
        const auto dash = token.find('-');
        if (dash == std::string::npos) {
            ports.push_back(number(token));
        } else {
            if (token.find('-', dash + 1) != std::string::npos) throw std::invalid_argument("Invalid port range: " + token);
            const int lo = number(token.substr(0, dash));
            const int hi = number(token.substr(dash + 1));
            if (lo > hi) throw std::invalid_argument("Port range must be ascending: " + token);
            for (int port = lo; port <= hi; ++port) ports.push_back(port);
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    std::sort(ports.begin(), ports.end());
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());
    return ports;
}

std::vector<Result> scan(const Options& options) {
    if (options.ports.empty()) throw std::invalid_argument("No ports selected");
    if (options.threads < 1 || options.threads > 256) throw std::invalid_argument("--threads must be in 1..256");
    if (options.timeout_ms < 10 || options.timeout_ms > 60000) throw std::invalid_argument("--timeout-ms must be in 10..60000");
    [[maybe_unused]] NetworkInit network;
    const sockaddr_in target = resolve_target(options);
    std::vector<Result> results(options.ports.size());
    std::atomic<std::size_t> index{0};
    const auto worker = [&] {
        for (;;) {
            const auto i = index.fetch_add(1);
            if (i >= options.ports.size()) break;
            results[i] = inspect(target, options.ports[i], options.timeout_ms);
        }
    };
    std::vector<std::thread> workers;
    const auto count = std::min(static_cast<std::size_t>(options.threads), options.ports.size());
    workers.reserve(count);
    for (std::size_t n = 0; n < count; ++n) workers.emplace_back(worker);
    for (auto& thread : workers) thread.join();
    return results;
}

std::string state_name(State state) {
    switch (state) {
        case State::Open: return "OPEN";
        case State::Closed: return "CLOSED";
        case State::Filtered: return "FILTERED";
        case State::Error: return "ERROR";
    }
    return "ERROR";
}

std::string service_hint(int port) {
    switch (port) {
        case 20: case 21: return "ftp";
        case 22: return "ssh";
        case 23: return "telnet";
        case 25: return "smtp";
        case 53: return "dns";
        case 80: case 8080: case 8000: return "http?";
        case 110: return "pop3";
        case 139: case 445: return "smb";
        case 143: return "imap";
        case 443: case 8443: return "https?";
        case 3306: return "mysql";
        case 3389: return "rdp";
        case 5432: return "postgresql";
        case 6379: return "redis";
        default: return "-";
    }
}

} // namespace bk
