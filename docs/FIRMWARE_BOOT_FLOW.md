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


## CPU abort exceptions and boot preparation

The ARM interpreter now routes supported instruction-fetch failures to the Prefetch Abort vector (0x0C) and supported data/MMU failures to the Data Abort vector (0x10). It saves CPSR in SPSR_abt, updates LR_abt, masks IRQ/async abort, and records simplified DFSR/IFSR and DFAR/IFAR values. Fault-status encodings are a limited model; nested aborts and full ARM1176 fault semantics are not implemented.

Use `c3350emu --prepare-boot <firmware-directory> [report.txt]` to inventory PSI, SLB, BSY, and CDS files in the observed downloader order. It checks component presence and records file sizes, classifications, and headers. This is a boot-preparation manifest, not an FLS decoder or firmware execution. Unknown addresses and entry points remain unknown.


## Executable multi-region boot image

`--run-boot-plan <boot.plan>` is the first multi-region execution path. A plan defines one shared RAM window, a CPU entry point, an instruction/step limit, and one or more explicit FLS byte ranges. All ranges are loaded before CPU reset and execution, allowing normal branch instructions to transfer control between regions without restarting the CPU. Relative file paths resolve against the plan file's directory. Invalid file ranges, RAM overflows, overlapping destinations, and entries outside loaded data are rejected. This is execution of user-selected raw FLS ranges, not automatic FLS decoding; all offsets and addresses remain user-supplied and unverified for the C3350.
