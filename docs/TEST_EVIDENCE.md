# Test Evidence and Result Interpretation

## Evidence provenance

The six images below were captured from **actual Linux Xterm windows** while the C++ executable and Python test scripts ran on November 19, 2026 (US Eastern). Screenshots were cropped from the X11 window capture, without replacing the terminal output with edited or generated text. The fixtures are synthetic/temporary but the socket connections and test executions are real.

**Build environment:** GCC 14.2, CMake 3.31.6, Python 3.13.5, local virtual display. This setup was not a production endpoint, and was not a cloud or enterprise network.

## Evidence matrix

| Image | Command | Observed outcome | What it proves |
| --- | --- | --- | --- |
| [01 build](../screenshots/01-cmake-build.png) | `cmake --build build --config Release` | C++ target built | Linux compilation succeeded |
| [02 live scan](../screenshots/02-localhost-open-closed.png) | `python3 scripts/demo_local.py --mode table` | One open, one closed localhost port | Connect scan handles a real socket listener and refusal |
| [03 JSON](../screenshots/03-json-report.png) | `python3 scripts/demo_local.py --mode json` | Valid summary and port entries | Structured reporting works in a live local run |
| [04 CSV](../screenshots/04-csv-report.png) | `python3 scripts/demo_local.py --mode csv` | CSV header and rows | Flat report formatting works |
| [05 tests](../screenshots/05-automated-tests.png) | `python3 -m unittest discover -s tests -v` | 9 tests passed | Checkable input and integration behavior |
| [06 guard](../screenshots/06-authorization-gate.png) | `python3 scripts/demo_local.py --mode guard` | Refusal with exit code 2 | Default non-loopback safety gate triggers |

## Automated checks

1. Help page is available.
2. An ephemeral loopback listener returns OPEN.
3. An unbound loopback port returns CLOSED.
4. Duplicate and unordered port specifications are normalized.
5. Invalid port specifications are rejected.
6. Non-loopback addresses are blocked without permission confirmation.
7. Thread and timeout values must be within documented ranges.
8. CSV file export contains the expected port and state.
9. JSON file export uses valid structured data.

CTest wraps the same Python test suite for CI. A passing local run is not evidence that Windows or all other environments have passed; that must be checked from GitHub Actions after publishing.

## Known uncertainty

- Local ephemeral ports are dynamic and can change between screenshots or runs.
- An unused port is assumed to stay unused while the command executes; on a busy host that assumption may fail.
- TCP timeouts are a hint of filtering, not proof of a specific firewall rule.
- No production, internet, or corporate scan was performed.
- No exploit, authentication bypass, vulnerability confirmation, or banner identification was attempted.
