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
| Memory package | K5W1G12ACP family; likely OneNAND + DDR MCP | C3350 service log gives family marking; a separate XMM2150 Samsung service schematic shows matching K5W1G12ACP-BL500 as a package with OneNAND and DDR interfaces. Exact C3350 suffix/configuration remains unverified |
| Bootloader | 2150_OD_KJ1_3.1 | Reported in a C3350 flashing log |
| Download protocol | 3.32 | Reported in a C3350 flashing log |
| Boot mode | BB | Reported in a C3350 flashing log |
| Flash ID | 003000EC | Reported in a C3350 flashing log |
| Download baud rate | 921600 | Reported in a C3350 flashing log |

## RAM research: confirmed versus unknown

Intel's XMM 2130/2150 platform brief identifies an **SRAM** block and lists
**NAND/LPDDR1** as the memory technology. This confirms that the platform includes
on-chip SRAM and an external memory subsystem, but the brief does not publish
physical base addresses, capacities, controller registers, or initialization
timings. It is not enough to construct a real GT-C3350 RAM map.

Source: Intel, *Mobile XMM 2130/2150 Platform Brief*, document 326687-001:
https://www.intel.com/content/dam/www/public/us/en/documents/platform-briefs/mobile-xmm-2130-2150-brief.pdf

A C3350-specific flashing report identifies flash ID **003000EC** and loader
**2150_OD_KJ1_3.1**, but does not report the CPU-visible RAM map:
https://forum.gsmhosting.com/vbb/f453/c3350-dead-after-flash-how-revive-solved-3100313/

A separate S3350/XMM6180 report contains flash write ranges such as 0x40000000.
Those are flash-storage operations on a different phone/platform and must **not**
be copied into the GT-C3350 RAM map:
https://forum.gsmhosting.com/vbb/f311/successfully-unlocked-flashed-phones-hwk-798344-print/index141.html

| Memory item | Current result |
|---|---|
| On-chip SRAM exists | Confirmed at platform-family level |
| External NAND/LPDDR1 subsystem | Confirmed at platform-family level |
| SRAM capacity and CPU address | Unknown |
| LPDDR capacity and CPU address | Unknown |
| LPDDR controller registers/timings | Unknown |
| RAM reset contents and aliases | Unknown |
| C3350 physical RAM map | Not established |

## Major memory-package clue: OneNAND and DDR may share one MCP

A C3350-specific service/unlock log reports the memory marking
`K5W1G12ACP(C3350)S`:
https://forum.gsmhosting.com/vbb/f453/gt-c3350-unlock-flash-done-1442539/

A Samsung GT-S5222 service manual documents a closely related XMM2150
(CatFish) platform. Its product-function section specifies
**1G OneNAND + 512M DDR SDRAM** and identifies the baseband as Intel
**CatFish XMM2150**. The schematic labels the memory package UME200 and shows
both OneNAND signals (`CS0n_OneNAND`) and a separate RAM chip-select
(`CS1n_RAM`), plus a 16-bit DDR data bus (`DDR_DQ(0:15)`) and DDR control
signals. Its parts list identifies UME200 as memory, Samsung part code
1108-000406; the schematic labels the package `K5W1G12ACP-BL500`.

Sources:
- GT-S5222 service manual, memory specification and XMM2150 platform: https://www.scribd.com/document/554100631/Manual-de-Servico-Tecnico-Samsung-GT-S5222
- C3350-specific K5W1G12ACP family marking: https://forum.gsmhosting.com/vbb/f453/gt-c3350-unlock-flash-done-1442539/

**Important correction:** the K5W1G12ACP package must not be treated as flash-only.
The matching XMM2150-family schematic shows OneNAND and DDR interfaces in the
same memory package. The earlier interpretation that this marking represented
only nonvolatile storage was incomplete. It is likely a multi-chip package
(MCP) combining OneNAND and DDR SDRAM, but the exact C3350 fitted variant still
needs direct board/schematic confirmation.

The sibling phone's "512M DDR SDRAM" specification is a useful capacity clue,
but the notation does not explicitly state bits versus bytes in that line.
Do not silently convert it to 64 MiB or 512 MiB in the C3350 profile. If it
means 512 Mbit, that equals 64 MiB; this remains a hypothesis for C3350, not a
confirmed C3350 capacity.

| Memory item | Current result |
|---|---|
| XMM2150 on-chip SRAM | Confirmed at platform-family level by Intel |
| C3350 memory marking | `K5W1G12ACP(C3350)S`, reported by C3350 service log |
| Closely matching XMM2150 package | `K5W1G12ACP-BL500`, shown on GT-S5222 schematic |
| OneNAND interface | Confirmed on the sibling XMM2150 schematic |
| Separate DDR RAM interface | Confirmed on the sibling XMM2150 schematic |
| Sibling phone DDR capacity | Listed as 512M DDR SDRAM; bit/byte interpretation not explicit |
| C3350 DDR capacity and CPU address | Still unverified |
| C3350 physical RAM map and init sequence | Still unknown |

## Explicitly unknown

- Exact ARM1176 implementation suffix (for example, JZ/JZF-S), silicon revision/rXpY, clock and boot exception state.
- CPU MIDR/CPUID value from the actual GT-C3350, needed to confirm the implemented core revision.
- Physical LPDDR capacity, base address, timing and initialization sequence.
- On-chip SRAM capacity, base address and boot-time visibility.
- Exact MCP package suffix, OneNAND geometry, DDR organization, host-interface configuration and register offsets.
- CPU-visible flash mapping and boot aliasing.
- Interrupt controller, timers, UART, LCD and keypad register maps.
- FLS record structure, segment checksums, compression and load/entry addresses.

## Firmware staging implementation

C3350EMU now has a **raw segment staging** operation. It accepts an explicit file
offset, byte length, RAM base/size, and destination address, maps a temporary
writable RAM region, copies the selected bytes, and reports a calculated CRC-32.
It rejects out-of-file ranges and unmapped destinations.

This is deliberately not called an FLS decoder. Until the container's record
format and payload offsets are verified, the user must supply the byte range.
The RAM base and size are also user-configured scratch values, not claimed
GT-C3350 physical addresses. The current command is a staging/test operation;
it does not yet start the CPU or preserve RAM after the process exits.

## Emulator design consequences

1. Model OneNAND and DDR as separate logical interfaces even if they are integrated in one MCP package.
2. Do not map OneNAND storage as ordinary writable RAM.
3. Do not hard-code a DDR capacity or CPU memory map from the sibling GT-S5222 specification.
4. Keep the DDR controller and memory initialization sequence unknown until C3350 evidence is available.
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
- FLS parser output identifying verified record boundaries, payload offsets, lengths and checksums.
- A verified reset vector or bootloader entry point.
