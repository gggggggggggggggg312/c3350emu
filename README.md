# C3350EMU

An experimental research emulator targeting the Samsung GT-C3350. The long-term objective is execution of original phone firmware.

## Status

This project is in the firmware-analysis and infrastructure stage. It does **not** currently boot the phone or execute its firmware.

Current functionality:
- C++ command-line application.
- Conservative FLS file inspection: file size and raw header bytes.
- Python firmware inspector: SHA-256, header dump, and printable-string sampling.
- Basic 32-bit address-space memory map with read/write permissions.
- Automated C++ tests through CTest and GitHub Actions.
- Windows and Linux CI builds.

The FLS parser is intentionally conservative. The complete Samsung FLS container structure, segment records, checksums, load addresses, and entry-point rules have not yet been verified. The current parser does not claim to extract executable sections.

## Hardware research notes

- Firmware package under investigation: `C3350XXKK1_OXEKK1`.
- Firmware strings include `XMM2150` and `xmm2150_BOOTLOADER_VERSION_3_1`.
- ARM-like instruction patterns were observed in a candidate PSI region. They must be validated against confirmed segment boundaries and execution addresses before selecting a CPU core.
- Do not infer the exact processor core, ARM version, memory map, or peripheral layout from the platform name alone.

## Repository layout

```
C3350EMU/
├── .github/workflows/build.yml
├── CMakeLists.txt
├── firmware/                 # Local firmware; binaries are not committed
├── src/
│   ├── main.cpp
│   ├── cpu/                  # CPU scaffold; execution not implemented
│   ├── firmware/             # Conservative FLS inspection
│   └── memory/               # Basic memory-region implementation
├── tests/
│   ├── test_fls_parser.cpp
│   └── test_memory.cpp
└── tools/
    ├── inspect_firmware.py
    └── disassemble_candidate.py
```

## Build on Windows

Install Visual Studio Build Tools with the C++ workload and CMake.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\c3350emu.exe --help
```

## Build on Linux

Install a C++17 compiler, CMake, and CTest.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/c3350emu --help
```

## Inspect firmware

The C++ program can report basic file metadata and the first 64 bytes:

```powershell
.\build\Release\c3350emu.exe --inspect-firmware firmware/PSI.fls
```

The Python tool provides additional analysis:

```powershell
py tools/inspect_firmware.py firmware/PSI.fls
```

It reports file size, SHA-256, a header hex dump, and a limited sample of printable strings. These are inspection aids, not a complete FLS decoder.

Optional disassembly experiment:

```powershell
py -m pip install -r requirements.txt
py tools/disassemble_candidate.py firmware/PSI.fls --offset 0x36000 --length 0x100
```

Treat candidate disassembly as a hypothesis until executable sections and addresses are established.

## GitHub Actions builds

Pushes and pull requests trigger Windows and Linux builds plus tests. Download the latest successful build from the repository's Actions tab under Artifacts:
- `C3350EMU-windows-x64`
- `C3350EMU-linux-x64`

Artifacts currently expire after 30 days. These are research tools, not a bootable phone emulator.

## Firmware files

Firmware binaries are intentionally excluded from Git. Only redistribute firmware when you have the legal right to do so. Keep original firmware locally and record hashes in research notes.

## License

No license has been selected. Until one is added, all rights are reserved by default.
