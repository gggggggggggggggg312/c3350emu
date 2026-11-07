"""Experimental disassembly of a candidate firmware region.

This assumes ARM little-endian A32 solely for investigation. It does not
validate that the selected bytes are executable code or establish the CPU core.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_LITTLE_ENDIAN


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("firmware_file", type=Path)
    parser.add_argument("--offset", type=lambda value: int(value, 0), default=0x36000)
    parser.add_argument("--length", type=lambda value: int(value, 0), default=0x100)
    args = parser.parse_args()

    data = args.firmware_file.read_bytes()

    if args.offset < 0 or args.length <= 0 or args.offset >= len(data):
        parser.error("Invalid offset or length")

    region = data[args.offset : args.offset + args.length]
    if not region:
        parser.error("Selected region is empty")

    disassembler = Cs(
        CS_ARCH_ARM,
        CS_MODE_ARM | CS_MODE_LITTLE_ENDIAN,
    )

    print(f"File offset: 0x{args.offset:X}")
    print(f"Bytes selected: {len(region)}")
    print("Mode: experimental ARM A32 little-endian assumption")
    print()

    for instruction in disassembler.disasm(region, args.offset):
        raw = instruction.bytes.hex(" ")
        print(
            f"{instruction.address:08X}  "
            f"{raw:<12}  "
            f"{instruction.mnemonic:<8} "
            f"{instruction.op_str}"
        )


if __name__ == "__main__":
    main()
