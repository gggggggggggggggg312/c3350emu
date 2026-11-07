# GT-C3350 hardware research

This document separates observed evidence from emulator assumptions. Do not turn an
unknown field into a guessed address or register value.

## Current evidence

The Intel XMM 2130/2150 platform brief explicitly labels the baseband CPU as **ARM1176**. This is stronger evidence than the earlier generic ARM11-family inference. It does not identify the implementation suffix (such as ARM1176JZ-S versus ARM1176JZF-S) or the silicon revision. Therefore C3350EMU should target the ARM1176 architecture family for now, while keeping the exact core revision marked unknown.

Source: Intel, *Mobile XMM 2130/2150 Platform Brief*, document 326687-001 (2009), CPU block/key-features diagram: https://www.intel.com/content/dam/www/public/us/en/documents/platform-briefs/mobile-xmm-2130-2150-brief.pdf


| Item | Finding | Confidence / limitation |
|---|---|---|
| Target | Samsung GT-C3350 | Confirmed |
| Baseband platform | Intel XMM2150 family | Supported by C3350 firmware strings and model-specific flashing evidence |
| CPU core | ARM1176 family | Intel's XMM 2130/2150 platform brief identifies the baseband CPU as ARM1176; exact core suffix and silicon revision are not stated |
| CPU architecture | ARMv6K-class ARM11 (provisional) | Consistent with ARM1176; verify against the GT-C3350 silicon identification or CPU MIDR before fixing implementation details |
| CPU core revision | Unknown | No C3350-specific MIDR/CPUID readout or XMM2150 revision record found yet |
| Instruction architecture | ARMv6 family | ARM11-family direction; validate against executable firmware regions |
| Flash marking | K5W1G12ACP(C3350)S | Reported in a GT-C3350 service log |
| Storage technology | MuxOneNAND candidate | Related part-number documentation points to MuxOneNAND; exact fitted suffix/variant must be checked |
| Bootloader | 2150_OD_KJ1_3.1 | Reported in a C3350 flashing log |
| Download protocol | 3.32 | Reported in a C3350 flashing log |
| Boot mode | BB | Reported in a C3350 flashing log |
| Flash ID | 003000EC | Reported in a C3350 flashing log |
| Download baud rate | 921600 | Reported in a C3350 flashing log |

## Explicitly unknown

- Exact ARM1176 implementation suffix (for example, JZ/JZF-S), silicon revision/rXpY, clock and boot exception state.
- CPU MIDR/CPUID value from the actual GT-C3350, needed to confirm the implemented core revision.
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
