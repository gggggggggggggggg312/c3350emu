"""Inspect firmware component size, hashes, and leading bytes."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


def inspect_file(path: Path) -> None:
    data = path.read_bytes()

    print(f"File:   {path}")
    print(f"Size:   {len(data):,} bytes")
    print(f"SHA256: {hashlib.sha256(data).hexdigest()}")
    print(f"First 64 bytes: {data[:64].hex(' ')}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("firmware_file", type=Path)
    args = parser.parse_args()

    if not args.firmware_file.is_file():
        parser.error(f"File does not exist: {args.firmware_file}")

    inspect_file(args.firmware_file)


if __name__ == "__main__":
    main()
