# C3350EMU

An experimental emulator project targeting the Samsung GT-C3350 (Xcover 2), with the long-term goal of executing original firmware.

## Project status

Early reverse-engineering / scaffolding stage. This repository does **not** yet boot the phone or execute its firmware.

Known investigation leads:
- Firmware package: `C3350XXKK1_OXEKK1`
- Platform string reported in the firmware: `XMM2150`
- Bootloader strings reported: `xmm2150_BOOTLOADER_VERSION_3_1` and `3.1_M1S1`
- Candidate ARM instruction patterns have been observed in a firmware region, but the exact CPU core, execution mode, memory map, and load addresses are not yet established.

Do not treat the platform name as proof of a CPU architecture. The CPU backend must be selected only after the firmware and boot chain have been properly analyzed.

## Technology

- Python: tooling, firmware inspection, orchestration, and later device models.
- C++: performance-sensitive CPU and memory components.
- Capstone: optional disassembly experiments.

## Repository layout

```
C3350EMU/
├── CMakeLists.txt
├── requirements.txt
├── firmware/                 # Put locally obtained firmware here; not tracked
├── src/
│   ├── main.py
│   └── cpu/
│       ├── cpu.cpp
│       └── cpu.hpp
└── tools/
    ├── inspect_firmware.py
    └── disassemble_candidate.py
```

## Getting started

1. Create a Python environment.
2. Install tooling dependencies:
   ```powershell
   py -m pip install -r requirements.txt
   ```
3. Place extracted firmware components in the local `firmware/` directory.
4. Inspect a component:
   ```powershell
   py tools/inspect_firmware.py firmware/C3350XXKK1_BIN_psi.fls
   ```
5. Optionally inspect a candidate region as ARM little-endian code:
   ```powershell
   py tools/disassemble_candidate.py firmware/C3350XXKK1_BIN_psi.fls --offset 0x36000 --length 0x100
   ```

The disassembly script is an experiment, not proof that the selected region is executable ARM code. Confirm container headers, segment boundaries, load addresses, and processor mode before relying on its output.

## Firmware files

Firmware binaries are intentionally excluded from Git. Only commit files you are legally allowed to redistribute. Keep original firmware locally and record hashes in investigation notes.

## License

No license has been selected yet. Until one is added, all rights are reserved by default.
