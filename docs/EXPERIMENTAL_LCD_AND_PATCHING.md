# Experimental LCD and firmware patching

C3350EMU now supports a memory-mapped device interface and an experimental 240x320 RGB666 LCD controller. This is a development device so the CPU can perform real MMIO reads and writes and the Windows GUI can display the framebuffer.

**Important:** the register layout and base address are not verified GT-C3350 hardware facts. Choose a non-overlapping base in the boot plan. Do not assume the original phone firmware already knows this interface.

## Boot-plan directives

```
LCD 0x70000000
PATCH 0x10001000 0010A0E3 0120A0E3
```

- `LCD base` maps the experimental 0x100-byte controller register window.
- `PATCH address expected-hex replacement-hex` replaces bytes in emulated RAM after segments and memory initialization have loaded. Both byte strings must have equal, non-zero lengths.
- Patch bytes are written in memory order. The expected bytes must match exactly or boot stops with a patch-precondition error.
- Patches affect the emulated memory image only. Original .fls/.bin files are never overwritten.

## LCD register window

| Offset | Access | Meaning |
| --- | --- | --- |
| 0x00 | R/W | Control latch (currently stored; no panel power/timing behavior yet) |
| 0x04 | R | Width in low 16 bits (240), height in high 16 bits (320) |
| 0x08 | R/W | Framebuffer pixel index, 0 through 76799 |
| 0x0C | R/W | 32-bit RGB666 pixel data; a full word write commits one pixel and advances the index |
| 0x10 | R | Ready bit (bit 0 = 1) |

The framebuffer is 240x320 pixels. RGB666 uses 6 bits each for red, green, and blue (low 18 bits of the data word: R in bits 12-17, G in bits 6-11, B in bits 0-5). The CPU can write the pixel index at base+0x08 and a 32-bit RGB666 word at base+0x0C. The GUI displays the resulting framebuffer after execution stops.

## What this does not emulate yet

This is not yet the original C3350 LCD controller. It does not implement the phone's actual controller ID, command/data protocol, timing, interrupts, DMA, panel initialization sequence, or physical pin behavior. Those need evidence from the real device or firmware. The generic MMIO plumbing is now in place so the correct controller can replace this development interface later.
