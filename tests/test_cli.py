"""Integration tests: only operate on an ephemeral 127.0.0.1 listener."""
import csv
import json
import os
from pathlib import Path
import socket
import subprocess
import tempfile
import unittest

BIN = Path(os.environ.get("SCANNER_BIN", "build/simple-port-scanner")).resolve()


class PortScannerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not BIN.is_file():
            raise RuntimeError(f"Build the scanner first: executable missing at {BIN}")

    def execute(self, *args):
        return subprocess.run([str(BIN), *map(str, args)], text=True, capture_output=True, timeout=15)

    def test_help_text(self):
        result = self.execute("--help")
        self.assertEqual(result.returncode, 0)
        self.assertIn("Authorized TCP Connect Port Scanner", result.stdout)

    def test_open_loopback_listener(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.bind(("127.0.0.1", 0))
            server.listen(2)
            port = server.getsockname()[1]
            result = self.execute("--target", "127.0.0.1", "--ports", port, "--format", "json")
            self.assertEqual(result.returncode, 0, result.stderr)
            payload = json.loads(result.stdout)
            self.assertEqual(payload["results"], [{"port": port, "state": "OPEN", "service_hint": "-", "detail": "TCP handshake completed"}])
            self.assertEqual(payload["summary"]["open"], 1)

    def test_closed_loopback_port(self):
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            port = reservation.getsockname()[1]
        result = self.execute("--ports", port, "--format", "json")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["results"][0]["state"], "CLOSED")

    def test_deduplicate_and_sort(self):
        result = self.execute("--ports", "443,80,80,79-80", "--format", "json")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual([x["port"] for x in json.loads(result.stdout)["results"]], [79, 80, 443])

    def test_reject_malformed_ranges(self):
        for ports in ("0", "65536", "1-", "88-80", "1,,2", "abc", "1-2-3"):
            with self.subTest(ports=ports):
                result = self.execute("--ports", ports)
                self.assertEqual(result.returncode, 2)
                self.assertIn("Error:", result.stderr)

    def test_reject_unapproved_remote_target(self):
        result = self.execute("--target", "192.0.2.10", "--ports", "80")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Non-loopback target blocked", result.stderr)

    def test_invalid_worker_and_timeout_bounds(self):
        for args in (("--threads", "0"), ("--threads", "257"), ("--timeout-ms", "1"), ("--timeout-ms", "60001")):
            with self.subTest(args=args):
                self.assertEqual(self.execute(*args).returncode, 2)

    def test_csv_export(self):
        with socket.socket() as server:
            server.bind(("127.0.0.1", 0))
            server.listen(1)
            port = server.getsockname()[1]
            with tempfile.TemporaryDirectory() as folder:
                file = Path(folder) / "results.csv"
                result = self.execute("--ports", port, "--format", "csv", "--output", file)
                self.assertEqual(result.returncode, 0, result.stderr)
                with file.open(newline="") as handle:
                    rows = list(csv.DictReader(handle))
                self.assertEqual(len(rows), 1)
                self.assertEqual(rows[0]["state"], "OPEN")
                self.assertEqual(rows[0]["port"], str(port))

    def test_json_export(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder) / "scan.json"
            result = self.execute("--ports", "22", "--format", "json", "--output", file)
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(file.read_text())
            self.assertEqual(data["scan_type"], "TCP connect")
            self.assertEqual(len(data["results"]), 1)


if __name__ == "__main__":
    unittest.main()
