# C3350EMU

An experimental research emulator targeting the Samsung GT-C3350. The long-term objective is execution of original phone firmware.

## Status

This project is in the firmware-analysis and infrastructure stage. It does **not** currently boot the phone or execute its firmware.

Current functionality:
- C++ command-line application.
- Provisional GT-C3350 hardware research profile, available with --platform-info.
- Conservative FLS file inspection: file size and raw header bytes.
- Python firmware inspector: SHA-256, header dump, and printable-string sampling.
- FlashTool E2 session-log analyzer: extracts item names, downloader regions, checksum/verification results and transfer times from text logs.
- Basic 32-bit address-space memory map with read/write permissions.
- Initial memory-backed ARM-state interpreter: data-processing instructions, branches, and basic immediate/register-offset single transfers.
- CPU unit tests for instruction execution, conditional execution, and memory access.
- Automated C++ tests through CTest and GitHub Actions.
- Windows and Linux CI builds.

The CPU interpreter is an early development component, not yet a verified XMM2150/GT-C3350 CPU model. It implements only a subset of ARM-state instructions; Thumb state, exceptions, coprocessors, MMU behavior, timing, and hardware-specific reset behavior are not implemented. The CPU architecture/core identification remains provisional. See docs/CPU_IMPLEMENTATION.md.\n\nThe FLS parser is intentionally conservative. The complete Samsung FLS container structure, segment records, checksums, load addresses, and entry-point rules have not yet been verified. The current parser does not claim to extract executable sections.

## Hardware research

Current evidence points to the Intel XMM2150 platform family and an ARM11 CPU family. A C3350 service log reports the memory marking K5W1G12ACP(C3350)S, while related part-number documentation suggests a MuxOneNAND device. The exact fitted memory variant and its register map still need confirmation.

A C3350 flashing log reports bootloader identifier 2150_OD_KJ1_3.1, protocol 3.32, boot mode BB, flash ID 003000EC, and a download sequence in which BSY is handled as two regions. These downloader region numbers are **not** treated as CPU physical addresses.

See:
- docs/HARDWARE_RESEARCH.md
- docs/FIRMWARE_BOOT_FLOW.md

The emulator deliberately does not guess physical RAM addresses, OneNAND register offsets, CPU clock, or peripheral addresses.

## Repository layout

```
C3350EMU/
├── .github/workflows/build.yml
├── CMakeLists.txt
├── docs/
│   ├── FIRMWARE_BOOT_FLOW.md
│   └── HARDWARE_RESEARCH.md
├── firmware/                 # Local firmware; binaries are not committed
├── src/
│   ├── main.cpp
│   ├── cpu/                  # CPU scaffold; execution not implemented
│   ├── firmware/             # Conservative FLS inspection
│   ├── memory/               # Basic memory-region implementation
│   └── platform/             # Provisional hardware research profile
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
.\build\Release\c3350emu.exe --platform-info
```

## Build on Linux

Install a C++17 compiler, CMake, and CTest.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/c3350emu --help
./build/c3350emu --platform-info
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
- C3350EMU-windows-x64
- C3350EMU-linux-x64

Artifacts currently expire after 30 days. These are research tools, not a bootable phone emulator.

## Analyze a FlashTool E2 log

Save a FlashTool E2 session log as a text file, then run:

```powershell
py tools/analyze_flashtool_log.py flash-session.txt
py tools/analyze_flashtool_log.py flash-session.txt --json
```

This analyzes the *log*, not the binary FLS container. Downloader region numbers
are not treated as CPU addresses. See docs/FLASHTOOL_LOG_ANALYSIS.md for the
observed C3350 evidence and limitations.

## Firmware files

Firmware binaries are intentionally excluded from Git. Only redistribute firmware when you have the legal right to do so. Keep original firmware locally and record hashes in research notes.

## License

No license has been selected. Until one is added, all rights are reserved by default.
