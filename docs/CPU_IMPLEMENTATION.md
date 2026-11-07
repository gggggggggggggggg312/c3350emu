# CPU interpreter status

C3350EMU now contains a memory-backed ARM/Thumb interpreter with selected ARMv6-era system features. It remains a partial model, not proof that the target's exact CPU core or reset state has been identified.

## Implemented instruction groups

- ARM condition-code evaluation (EQ through AL, with NV not executed), plus ARM/Thumb interworking.
- Data-processing operations: AND, EOR, SUB, RSB, ADD, TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN.
- Immediate operands with rotated 8-bit constants.
- Register operands with immediate shifts (LSL, LSR, ASR, ROR).
- B, BL, BX, and selected BLX branches.
- Basic single data transfers: LDR, STR, LDRB, STRB, immediate offsets, and unshifted register offsets.
- NZCV updates for the implemented arithmetic/test operations.
- Selected Thumb-1 operations, exception entry/return, banked registers, CP15, MMU/TLB, simplified caches, exclusive accesses (LDREX/STREX/CLREX), and ARMv6 REV/REV16/REVSH.

## Deliberate limitations

- Thumb support is a subset; Thumb-2 and many ARMv6/ARM1176 instructions remain unimplemented.
- Interrupt delivery, timers, cycle timing, full ARM1176 system-control behavior, and complete exception/abort semantics remain incomplete.
- Many multiply, block-transfer, coprocessor, DSP, and system instructions remain unsupported.
- Register-specified shifts and register-shifted transfer offsets are not implemented.
- Reset CPSR value is a temporary test default, not a verified GT-C3350 reset value.
- No physical RAM/ROM addresses or peripheral registers are assumed.

The interpreter currently exists to establish a tested execution foundation. It must be validated against an identified CPU architecture and known instruction traces before it is used to interpret the phone's firmware.

## Test

The `cpu_tests` CTest target executes a tiny synthetic program that moves values, adds, stores, loads, checks conditional execution, and verifies that unmapped instruction fetches fail.

Build and run:

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Next emulator milestones

1. Confirm the exact CPU architecture and execution states from reliable C3350/XMM2150 evidence.
2. Add instruction tests against a trusted ARM reference or known-good trace.
3. Add Thumb execution if the target uses it.
4. Parse actual firmware sections and establish their load/entry addresses from binary evidence.
5. Connect the CPU to a verified memory map and progressively implement required peripherals.


## ARM exclusive accesses

- ARM word `LDREX` establishes a local exclusive reservation for the translated physical word.
- ARM word `STREX` writes only when the reservation matches, returning status 0 on success and 1 on failure.
- Any CPU-originated store clears the local reservation; reset also clears it.
- Unaligned word-exclusive accesses and PC-register forms are rejected.
- This is a simplified local monitor. It does not model external bus masters, multiprocessor shareability, cache-coherency protocols, or every ARM1176 exclusive-monitor rule.


## ARM-to-Thumb BLX immediate

The ARM interpreter supports the unconditional BLX-immediate encoding. It calculates the signed PC-relative target including the H bit, writes the ARM return address to LR, switches CPSR to Thumb state, and continues at the halfword-aligned target. This is a useful ARMv5T interworking path for mixed ARM/Thumb boot code; it does not establish that a particular C3350 firmware image uses this instruction.


The interpreter also supports BLX-register in ARM state and the Thumb high-register BLX form. Both preserve the caller's instruction-set state in LR and select the target state from the target address bit 0. These paths are tested with ARM-to-Thumb, Thumb-to-ARM, and return transitions.


## ARMv6 byte-reversal operations

ARM-state and Thumb encodings of REV, REV16, and REVSH are implemented and covered by synthetic instruction tests. REV reverses all four bytes, REV16 reverses bytes within each 16-bit halfword, and REVSH reverses the low halfword then sign-extends it. These ARMv6 operations can appear in compiler-generated low-level routines; their presence in this emulator does not establish that a specific C3350 firmware path requires them.


## ARM exclusive-monitor control

The ARM unconditional CLREX encoding clears the CPU's local exclusive reservation. A subsequent STREX therefore reports failure unless a new LDREX establishes a reservation. This models the local monitor only; external agents and multicore coherency are not represented.
