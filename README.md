# C3350EMU

An experimental research emulator targeting the Samsung GT-C3350. The long-term objective is execution of original phone firmware.

## Status

This project is in the firmware-analysis and infrastructure stage. It does **not** currently boot the phone or execute its firmware.

Current functionality:
- Native Windows GUI target (`c3350emu-gui`) for selecting firmware/binary files, viewing header bytes, and running a user-configured raw ARM image.
- Separate simplified ARM instruction and data cache models with CP15 cache invalidation operations.
- C++ command-line application.\n- Simulated battery subsystem with percentage, voltage estimate, charging/discharging, and elapsed-time progression (`--battery-demo`).
- ARM Data Abort and Prefetch Abort exception entry with simplified CP15 fault registers.
- Firmware-set boot-preparation report for PSI, SLB, BSY, and CDS (`--prepare-boot`).
- Multi-region boot-plan loader: stages multiple explicitly selected FLS byte ranges into one shared RAM map, then executes from a chosen entry point so branches can hand off between loaded regions (`--run-boot-plan`).
- Provisional GT-C3350 hardware research profile, available with --platform-info.
- Conservative FLS file inspection: file size and raw header bytes.
- Python firmware inspector: SHA-256, header dump, and printable-string sampling.
- FlashTool E2 session-log analyzer: extracts item names, downloader regions, checksum/verification results and transfer times from text logs.
- Basic 32-bit address-space memory map with read/write permissions.
- Initial memory-backed ARM-state interpreter: data-processing instructions, branches, and basic immediate/register-offset single transfers.
- CPU unit tests for instruction execution, conditional execution, and memory access.
- Automated C++ tests through CTest and GitHub Actions.
- Windows and Linux CI builds.

The GUI's raw execution mode requires a user-supplied RAM map and entry point. It does not decode FLS containers automatically. The instruction/data caches are simplified 128-line, 32-byte, direct-mapped, write-through models; they are not a cycle-accurate ARM1176 cache implementation.

The CPU interpreter is an early development component, not yet a complete XMM2150/GT-C3350 CPU model. Intel's XMM 2130/2150 platform brief identifies the baseband CPU family as ARM1176; the exact ARM1176 implementation suffix and silicon revision remain unconfirmed. The interpreter implements only a subset of ARM-state instructions; Thumb state, exceptions, coprocessors, MMU behavior, timing, and hardware-specific reset behavior are not implemented. See docs/CPU_IMPLEMENTATION.md and docs/HARDWARE_RESEARCH.md.\n\nThe FLS parser is intentionally conservative. The complete Samsung FLS container structure, segment records, checksums, load addresses, and entry-point rules have not yet been verified. The current parser does not claim to extract executable sections.

## Hardware research

Current evidence points to the Intel XMM2150 platform family with an ARM1176 CPU family. A C3350 service log reports the memory marking K5W1G12ACP(C3350)S, while related part-number documentation suggests a MuxOneNAND device. The exact fitted memory variant and its register map still need confirmation.

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
.\build\Release\c3350emu-gui.exe
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

## Native firmware analysis (no Python required)

Use the built-in C++ analyzer to create a text report next to a firmware file:

```powershell
.\build\Release\c3350emu.exe --analyze-firmware firmware\BSY.fls
```

By default, the report is written as `BSY.fls.analysis.txt`. You can choose a report path:

```powershell
.\build\Release\c3350emu.exe --analyze-firmware firmware\BSY.fls bsy-report.txt
```

Run the same command for PSI, SLB, each BSY file/region supplied by the flashing package, and CDS. The report includes whole-file CRC-32 and FNV-1a identifiers, a header dump, 4 KiB entropy blocks, printable strings, long repeated-byte runs, pointer-shaped aligned words, and ARM branch-shaped words.

**Important:** these are forensic heuristics, not a verified Samsung FLS decoder. The calculated CRC-32 is for identifying/checking the whole sample, not proof of an embedded firmware checksum. Candidate words are not confirmed load addresses or entry points. The report alone cannot establish the container layout; comparing multiple real firmware components is necessary.

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

C3350EMU is licensed under the [MIT License](LICENSE). You may use, modify, and redistribute the project under the terms of that license. The license applies to this repository's original code and documentation; third-party components and firmware remain subject to their own licenses and redistribution restrictions.


## Battery simulation

The emulator includes a deterministic battery model for power-management experiments:

```powershell
.\\build\\Release\\c3350emu.exe --battery-demo 35 charge 3600
.\\build\\Release\\c3350emu.exe --battery-demo 80 discharge 1800
```

The current default model uses a provisional 1000 mAh capacity, 300 mA charging rate, and 180 mA discharge rate. Voltage is an approximate state-of-charge estimate from 3.30 V to 4.20 V. These are simulation parameters, **not verified GT-C3350 battery specifications or real battery telemetry**.


Prepare a local firmware set without guessing physical load addresses:

```powershell
.\\build\\Release\\c3350emu.exe --prepare-boot C:\\Users\\hp\\Downloads\\C3350XXKK1_OXEKK1 boot-preparation.txt
```

The report inventories expected components in the observed downloader order. It does not decode FLS records or boot the handset.


## Multi-region firmware execution

Create a UTF-8 text plan (for example `boot.plan`) next to the firmware files:

```text
# Values below are examples only, not verified C3350 addresses.
RAM 0x01000000 0x00200000
ENTRY 0x01000000
LIMIT 100000
TRACE "boot-trace.csv"
SEGMENT "C3350XXKK1_BIN_psi.fls" 0x1000 0x20000 0x01000000
SEGMENT "C3350XXKK1_BIN_slb.fls" 0x2000 0x30000 0x01020000
SEGMENT "bsy.fls" 0x3000 0x40000 0x01050000
SEGMENT "C3350XXKK1_BIN_CDS.fls" 0x4000 0x50000 0x01090000
```

Run `c3350emu.exe --run-boot-plan boot.plan`. Each SEGMENT line is `file offset length load-address`; paths are relative to the plan file. The loader verifies ranges, rejects overlapping RAM destinations, stages all selected bytes into one shared memory map, and starts one CPU at ENTRY. Optional `TRACE "boot-trace.csv"` records executed PCs, ARM/Thumb state, opcodes, source firmware filename and source offset, next PC, and CPSR. The CPU can branch between the staged regions. Replace every example offset, length, address, and entry with verified values before using real firmware. The example numbers are deliberately placeholders and are not claimed to match the Samsung FLS format.
