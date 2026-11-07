# FlashTool E2 log analysis

## Confirmed from a GT-C3350 session log

A public GT-C3350 flashing session records the following component order:

| Component | Source filename | Observed operation |
|---|---|---|
| PSI | C3350XXKK1_BIN_psi.fls | One write |
| SLB | C3350XXKK1_BIN_slb.fls | One write |
| BSY | C3350XXKK1_BIN_BSY.fls | Two writes are shown |
| CDS | C3350XXKK1_BIN_CDS.fls | One write |
| CSC | C3350OXAKK1_CSC.dffs | One write |

A more detailed FlashTool E2 log labels PSI as BOOT CORE, and BSY/CDS as CODE.
It reports a BSY transfer with downloader regions 0 and 1, followed by
`Checksum OK (0x0000)` and `Verify OK`.

## Important interpretation

- The log proves the downloader processes two regions for the BSY component.
- It does not prove that region 0 or region 1 is a CPU physical address.
- `Checksum OK (0x0000)` is a reported transfer/checksum result. It does not
  establish that 0x0000 is a checksum field in the FLS file.
- A downloader log is not a binary format specification.

## New log parser

Run:

```powershell
py tools/analyze_flashtool_log.py flash-session.txt
py tools/analyze_flashtool_log.py flash-session.txt --json
```

The script extracts component names, source filenames, downloader region numbers,
checksum results, verification status and transfer times. It does not parse or
modify firmware binaries.

## Research sources

- GSM-Forum, “GT-C3350 Unlock/Flash Done..!” (13 March 2012): component filenames
  and successful write sequence.
- GSM-Forum, “C3350 dead after flash. How to revive?” (7 December 2022): detailed
  FlashTool E2 log including bootloader identifier, protocol, flash ID, per-item
  region operations, checksum and verification messages.

These are community repair logs, not official Samsung protocol specifications.
