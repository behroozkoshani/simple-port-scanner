#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bk {

enum class State { Open, Closed, Filtered, Error };

struct Result {
    int port;
    State state;
    std::string detail;
};

struct Options {
    std::string target = "127.0.0.1";
    std::vector<int> ports;
    int threads = 32;
    int timeout_ms = 350;
    bool allow_remote = false;
};

std::vector<int> parse_ports(const std::string& spec);
std::vector<Result> scan(const Options& options);
std::string state_name(State state);
std::string service_hint(int port);

} // namespace bk
