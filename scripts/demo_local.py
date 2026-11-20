#!/usr/bin/env python3
"""Run reproducible live TCP scans entirely inside localhost (127.0.0.1)."""
import argparse
import os
from pathlib import Path
import socket
import subprocess
import sys

BASE = Path(__file__).resolve().parents[1]
BIN = Path(os.environ.get("SCANNER_BIN", BASE / "build" / "simple-port-scanner"))


def execute(args, expect=0):
    display_args = [str(arg) for arg in args]
    if Path(display_args[0]) == BIN:
        display_args[0] = "./build/simple-port-scanner"
    elif Path(display_args[0]) == Path(sys.executable):
        display_args[0] = "python3"
    print("$ " + " ".join(display_args), flush=True)
    result = subprocess.run(list(map(str, args)), cwd=BASE, text=True, capture_output=True, timeout=30)
    print(result.stdout, end="", flush=True)
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr, flush=True)
    if result.returncode != expect:
        raise SystemExit(f"Expected exit {expect}, got {result.returncode}")


def demo(format_):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 0))
        listener.listen(8)
        open_port = listener.getsockname()[1]
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as temp:
            temp.bind(("127.0.0.1", 0))
            closed_port = temp.getsockname()[1]
        print("Lab: isolated 127.0.0.1 socket listener, synthetic local test only")
        print(f"Fixture port: {open_port} (OPEN); unused port: {closed_port} (CLOSED)\n")
        execute([BIN, "--target", "127.0.0.1", "--ports", f"{open_port},{closed_port}",
                 "--threads", "4", "--timeout-ms", "500", "--format", format_])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--mode", choices=["table", "json", "csv", "guard", "tests", "build"], default="table")
    mode = parser.parse_args().mode
    if mode in ("table", "json", "csv"):
        demo(mode)
    elif mode == "guard":
        print("Demonstrating the opt-in safety gate. No remote scan is performed.\n")
        execute([BIN, "--target", "192.0.2.10", "--ports", "80"], expect=2)
    elif mode == "tests":
        execute([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"])
    elif mode == "build":
        execute(["cmake", "--build", "build", "--config", "Release"])


if __name__ == "__main__":
    main()
