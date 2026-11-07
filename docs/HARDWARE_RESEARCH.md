# GT-C3350 hardware research

This document separates observed evidence from emulator assumptions. Do not turn an
unknown field into a guessed address or register value.

## Current evidence

| Item | Finding | Confidence / limitation |
|---|---|---|
| Target | Samsung GT-C3350 | Confirmed |
| Baseband platform | Intel XMM2150 family | Supported by C3350 firmware strings and model-specific flashing evidence |
| CPU family | ARM11 | XMM2150 family documentation; exact core SKU/revision not established |
| Instruction architecture | ARMv6 family | ARM11-family direction; validate against executable firmware regions |
| Flash marking | K5W1G12ACP(C3350)S | Reported in a GT-C3350 service log |
| Storage technology | MuxOneNAND candidate | Related part-number documentation points to MuxOneNAND; exact fitted suffix/variant must be checked |
| Bootloader | 2150_OD_KJ1_3.1 | Reported in a C3350 flashing log |
| Download protocol | 3.32 | Reported in a C3350 flashing log |
| Boot mode | BB | Reported in a C3350 flashing log |
| Flash ID | 003000EC | Reported in a C3350 flashing log |
| Download baud rate | 921600 | Reported in a C3350 flashing log |

## Explicitly unknown

- Exact ARM11 core model, revision, clock and boot exception state.
- Physical DDR capacity, base address, timing and initialization sequence.
- Exact OneNAND part suffix, geometry, host interface configuration and register offsets.
- CPU-visible flash mapping and boot aliasing.
- Interrupt controller, timers, UART, LCD and keypad register maps.
- FLS record structure, segment checksums, compression and load/entry addresses.

## Emulator design consequences

1. Keep CPU, RAM, flash storage and peripheral buses as separate components.
2. Do not map flash as ordinary writable RAM.
3. Do not hard-code a memory map until boot code or reliable documentation establishes it.
4. Implement OneNAND commands only after obtaining the exact device datasheet and confirming the fitted variant.
5. Keep every reverse-engineered address tied to a source, firmware offset, or reproducible trace.

## Reported download sequence

A C3350 flashing log reports these component/region operations:

- PSI: region 0
- SLB: region 0
- BSY: region 0 and region 1
- CDS: region 0

This is a downloader operation sequence. It does not prove that these are CPU memory regions or reveal their load addresses. The emulator must not interpret the region numbers as physical addresses.

## Next evidence needed

- A readable datasheet for the exact K5W1G12ACP(C3350)S fitted component.
- Raw PSI, SLB, BSY and CDS files with hashes.
- FLS parser output identifying record boundaries and checksums.
- A verified reset vector or bootloader entry point.
