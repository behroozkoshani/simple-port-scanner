# Windows Build and Testing

The source uses conditional Winsock2 APIs on Windows, while the included evidence screenshots and tested compiler build are from Linux. The GitHub Actions workflow includes a Windows runner for independent build verification after publishing.

## Requirements

- Visual Studio 2022 Build Tools with Desktop development with C++ (or equivalent C++20 compiler)
- CMake 3.16+
- Python 3.9+ for test suite

From a Developer PowerShell or Visual Studio developer prompt:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\simple-port-scanner.exe --help
.\build\Release\simple-port-scanner.exe --target 127.0.0.1 --ports 22,80,443
ctest --test-dir build -C Release --output-on-failure
```

CMake links `ws2_32` for Windows. CTest passes the correct build path to Python via `SCANNER_BIN`, even on multi-configuration Windows builds. The demonstration script's default path is Linux-oriented; on Windows set `SCANNER_BIN` to `build\Release\simple-port-scanner.exe` before running it, or use the executable directly.

**Important:** Windows support is designed into the source and workflow but has not been verified on a Windows machine during the local evidence capture.
