#!/usr/bin/env python3
"""Create a reversible emulator-specific copy of the Samsung C3350 PSI image.

This tool deliberately refuses to guess hardware patches. It verifies every
expected byte before writing, never edits the source image, and can scan the
known candidate PSI entry area to prepare the next patch safely.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def scan(data: bytes, start: int, count: int, virtual_base: int) -> None:
    end = min(len(data), start + count * 4)
    if start < 0 or start >= len(data):
        raise SystemExit(f"Scan offset 0x{start:X} is outside the file.")
    print(f"PSI scan: file+0x{start:X} .. file+0x{end:X}")
    print("Offset      ARM word   Branch target (if B/BL)")
    for off in range(start, end - 3, 4):
        word = struct.unpack_from("<I", data, off)[0]
        pc = virtual_base + (off - start)
        target = ""
        if (word >> 25) & 0x7 == 0b101:
            imm = word & 0x00FFFFFF
            if imm & 0x00800000:
                imm -= 0x01000000
            dest = (pc + 8 + (imm << 2)) & 0xFFFFFFFF
            target = f"  -> 0x{dest:08X} ({'BL' if word & 0x01000000 else 'B'})"
        print(f"0x{off:08X}  0x{word:08X}{target}")


def apply_patches(source: Path, output: Path, manifest: Path) -> None:
    original = source.read_bytes()
    spec = json.loads(manifest.read_text(encoding="utf-8"))
    patches = spec.get("patches", [])
    if not patches:
        raise SystemExit("Manifest has no patches. Refusing to create a fake 'patched' image.")
    image = bytearray(original)
    for index, patch in enumerate(patches, 1):
        offset = int(patch["file_offset"], 0) if isinstance(patch["file_offset"], str) else int(patch["file_offset"])
        expected = bytes.fromhex(patch["expected_hex"])
        replacement = bytes.fromhex(patch["replacement_hex"])
        if not expected or len(expected) != len(replacement):
            raise SystemExit(f"Patch {index}: expected/replacement must be non-empty and equal length.")
        actual = bytes(image[offset:offset + len(expected)])
        if actual != expected:
            raise SystemExit(
                f"Patch {index} rejected at file+0x{offset:X}: "
                f"expected {expected.hex(' ')}, found {actual.hex(' ')}"
            )
        image[offset:offset + len(replacement)] = replacement
        print(f"Applied patch {index} at file+0x{offset:X}: {patch.get('reason', 'no reason supplied')}")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(image)
    print(f"Original SHA256: {sha256(original)}")
    print(f"Patched  SHA256: {sha256(image)}")
    print(f"Output: {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Safely patch a copy of the C3350 PSI firmware")
    parser.add_argument("psi", type=Path, help="Original C3350XXKK1_BIN_psi.fls")
    parser.add_argument("--scan", action="store_true", help="Print ARM words around the candidate PSI entry")
    parser.add_argument("--offset", type=lambda x: int(x, 0), default=0xCF8, help="File offset to scan (default: 0xCF8)")
    parser.add_argument("--count", type=int, default=48, help="Number of 32-bit words to scan")
    parser.add_argument("--virtual-base", type=lambda x: int(x, 0), default=0xCF8, help="Candidate runtime address corresponding to scan offset")
    parser.add_argument("--manifest", type=Path, help="JSON file containing verified byte patches")
    parser.add_argument("--output", type=Path, help="Output copy path; required with --manifest")
    args = parser.parse_args()
    if not args.psi.is_file():
        raise SystemExit(f"PSI file not found: {args.psi}")
    data = args.psi.read_bytes()
    print(f"Input: {args.psi} ({len(data):,} bytes)")
    print(f"SHA256: {sha256(data)}")
    if args.scan:
        scan(data, args.offset, args.count, args.virtual_base)
    if args.manifest:
        if not args.output:
            raise SystemExit("--output is required with --manifest")
        if args.output.resolve() == args.psi.resolve():
            raise SystemExit("Output must be a different file. The original firmware is never overwritten.")
        apply_patches(args.psi, args.output, args.manifest)
    elif not args.scan:
        parser.error("Choose --scan or --manifest")


if __name__ == "__main__":
    main()
