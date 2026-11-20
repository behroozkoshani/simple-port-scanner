#include "scanner.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int integer(const std::string& input, const std::string& name) {
    try {
        std::size_t length = 0;
        const int value = std::stoi(input, &length);
        if (length == input.size()) return value;
    } catch (const std::exception&) { }
    throw std::invalid_argument(name + " must be an integer");
}

void usage() {
    std::cout << R"(Behrooz Koshani | Authorized TCP Connect Port Scanner

Usage:
  simple-port-scanner [options]

Options:
  --target HOST          IPv4 address or hostname (default: 127.0.0.1)
  --ports SPEC           Ports and ranges, e.g. 22,80,443,8000-8005
  --threads N            Parallel connections, 1-256 (default: 32)
  --timeout-ms N         Per-port timeout, 10-60000 ms (default: 350)
  --format table|json|csv  Output format (default: table)
  --output FILE          Save output to file instead of stdout
  --allow-remote         Confirm authorization for non-loopback target
  --help                 Show this help

Scope:
  TCP connect scans only. Default target is localhost. No stealth probes,
  vulnerability exploitation, credential checks, or banner collection.
  Scan only systems you own or have explicit authorization to test.
)";
}

std::string escape_json(const std::string& text) {
    std::string result;
    for (const unsigned char c : text) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: if (c < 32) result += "?"; else result += static_cast<char>(c);
        }
    }
    return result;
}

void render(std::ostream& out, const bk::Options& opts, const std::string& format,
            const std::vector<bk::Result>& findings, long long elapsed_ms) {
    std::size_t open = 0, closed = 0, filtered = 0, errors = 0;
    for (const auto& f : findings) {
        if (f.state == bk::State::Open) ++open;
        if (f.state == bk::State::Closed) ++closed;
        if (f.state == bk::State::Filtered) ++filtered;
        if (f.state == bk::State::Error) ++errors;
    }
    if (format == "json") {
        out << "{\n  \"target\": \"" << escape_json(opts.target) << "\",\n"
            << "  \"scan_type\": \"TCP connect\",\n"
            << "  \"authorized_local_demo\": " << (opts.allow_remote ? "false" : "true") << ",\n"
            << "  \"duration_ms\": " << elapsed_ms << ",\n"
            << "  \"summary\": {\"open\": " << open << ", \"closed\": " << closed
            << ", \"filtered\": " << filtered << ", \"error\": " << errors << "},\n"
            << "  \"results\": [\n";
        for (std::size_t i = 0; i < findings.size(); ++i) {
            const auto& f = findings[i];
            out << "    {\"port\": " << f.port << ", \"state\": \"" << bk::state_name(f.state)
                << "\", \"service_hint\": \"" << bk::service_hint(f.port)
                << "\", \"detail\": \"" << escape_json(f.detail) << "\"}"
                << (i + 1 == findings.size() ? "" : ",") << "\n";
        }
        out << "  ]\n}\n";
    } else if (format == "csv") {
        out << "port,state,service_hint,detail\n";
        for (const auto& f : findings)
            out << f.port << ',' << bk::state_name(f.state) << ','
                << bk::service_hint(f.port) << ',' << f.detail << '\n';
    } else {
        out << "BK PORT SCANNER | authorized TCP connect scan\n"
            << "Target: " << opts.target << " | Ports: " << findings.size()
            << " | Workers: " << opts.threads << " | Timeout: " << opts.timeout_ms << " ms\n\n"
            << std::left << std::setw(8) << "PORT" << std::setw(11) << "STATE"
            << std::setw(12) << "SERVICE*" << "DETAIL\n"
            << "------------------------------------------------------------\n";
        for (const auto& f : findings)
            out << std::left << std::setw(8) << f.port << std::setw(11) << bk::state_name(f.state)
                << std::setw(12) << bk::service_hint(f.port) << f.detail << '\n';
        out << "\nSummary: " << open << " open | " << closed << " closed | " << filtered
            << " filtered | " << errors << " error | " << elapsed_ms << " ms\n"
            << "*Service names are port-based hints; no application identity verification.\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        bk::Options options;
        std::string format = "table";
        std::string output;
        std::string ports = "22,80,443,8080";
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") { usage(); return 0; }
            if (arg == "--allow-remote") { options.allow_remote = true; continue; }
            if (i + 1 >= argc) throw std::invalid_argument("Missing value for " + arg);
            const std::string value = argv[++i];
            if (arg == "--target") options.target = value;
            else if (arg == "--ports") ports = value;
            else if (arg == "--threads") options.threads = integer(value, arg);
            else if (arg == "--timeout-ms") options.timeout_ms = integer(value, arg);
            else if (arg == "--format") format = value;
            else if (arg == "--output") output = value;
            else throw std::invalid_argument("Unknown option: " + arg);
        }
        if (format != "table" && format != "json" && format != "csv")
            throw std::invalid_argument("--format must be table, json or csv");
        options.ports = bk::parse_ports(ports);
        const auto start = std::chrono::steady_clock::now();
        const auto findings = bk::scan(options);
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (output.empty()) render(std::cout, options, format, findings, elapsed_ms);
        else {
            std::ofstream file(output);
            if (!file) throw std::runtime_error("Unable to write output file: " + output);
            render(file, options, format, findings, elapsed_ms);
            if (!file) throw std::runtime_error("Failed while writing output file: " + output);
            std::cout << "Saved " << findings.size() << " TCP port results to " << output << '\n';
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\nUse --help for options.\n";
        return 2;
    }
}
