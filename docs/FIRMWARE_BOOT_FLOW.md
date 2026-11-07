# Firmware download and boot-flow notes

## Observed downloader order

The available GT-C3350 flashing log reports the following operations:

| Order | Component | Downloader region |
|---:|---|---:|
| 1 | PSI | 0 |
| 2 | SLB | 0 |
| 3 | BSY | 0 |
| 4 | BSY | 1 |
| 5 | CDS | 0 |

The log also reports bootloader identifier 2150_OD_KJ1_3.1, boot mode BB,
protocol 3.32, flash ID 003000EC, and baud rate 921600.

## What this does and does not tell us

It establishes a useful observed download sequence and that BSY is handled in two
separate downloader regions. It does not establish:

- the internal FLS container format;
- whether region numbers represent flash partitions, download buffers, or another
  protocol concept;
- CPU-visible load addresses;
- executable entry points;
- the runtime order in which the phone executes PSI, SLB, BSY or CDS.

## Implementation rule

Treat the sequence as metadata for research and logging only. Do not copy region
numbers into the emulator memory map. A future FLS decoder should emit explicit
records containing file offset, record type, payload length, checksum result and
any decoded address fields, with unknown fields left unknown.
