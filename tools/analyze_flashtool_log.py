"""Extract observations from Samsung FlashTool E2 session logs.

This parses log text, not Samsung .fls binary files. Region numbers are kept as
downloader labels and are never interpreted as CPU addresses.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

ITEM_RE = re.compile(
    r"Start downloading item ['\"](?P<kind>[^:'\"]+):(?P<name>[^'\"]+)['\"]"
    r".*?from file ['\"](?P<path>[^'\"]+)['\"]",
    re.IGNORECASE,
)
REGION_RE = re.compile(r"Load region\s+(\d+)", re.IGNORECASE)
CHECKSUM_RE = re.compile(r"Checksum OK\s*\(\s*(0x[0-9a-f]+)\s*\)", re.IGNORECASE)
TIME_RE = re.compile(r"Process time was\s+(.+?)(?:\.(?=\s*$)|$)", re.IGNORECASE)


def parse_log(text: str) -> list[dict[str, Any]]:
    """Return one record per downloaded firmware item."""
    records: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None

    for line in text.splitlines():
        item_match = ITEM_RE.search(line)
        if item_match:
            if current is not None:
                records.append(current)
            current = {
                "kind": item_match.group("kind").strip(),
                "item_name": item_match.group("name").strip(),
                "source_file": re.split(r"[\\/]", item_match.group("path"))[-1],
                "regions": [],
                "checksum": None,
                "verified": False,
                "process_time": None,
            }
            continue

        if current is None:
            continue

        region_match = REGION_RE.search(line)
        if region_match:
            current["regions"].append(int(region_match.group(1)))
            continue

        checksum_match = CHECKSUM_RE.search(line)
        if checksum_match:
            current["checksum"] = checksum_match.group(1).upper()
            continue

        if re.search(r"\bVerify OK\b", line, re.IGNORECASE):
            current["verified"] = True
            continue

        time_match = TIME_RE.search(line)
        if time_match:
            current["process_time"] = time_match.group(1).strip()

    if current is not None:
        records.append(current)

    return records


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Summarize Samsung FlashTool E2 logs (not .fls binaries)."
    )
    parser.add_argument("log_file", type=Path)
    parser.add_argument("--json", action="store_true", help="Output JSON")
    args = parser.parse_args()

    if not args.log_file.is_file():
        parser.error(f"Log file does not exist: {args.log_file}")

    records = parse_log(args.log_file.read_text(encoding="utf-8", errors="replace"))
    if args.json:
        print(json.dumps(records, indent=2))
        return

    if not records:
        print("No firmware download records found.")
        return

    for index, record in enumerate(records, 1):
        regions = ", ".join(str(value) for value in record["regions"]) or "not logged"
        print(f"{index}. {record['kind']} / {record['item_name']}")
        print(f"   File: {record['source_file']}")
        print(f"   Downloader regions: {regions}")
        print(f"   Checksum: {record['checksum'] or 'not logged'}")
        print(f"   Verify: {'OK' if record['verified'] else 'not logged'}")
        print(f"   Time: {record['process_time'] or 'not logged'}")

    print("\nNote: downloader region numbers are not physical CPU addresses.")


if __name__ == "__main__":
    main()
