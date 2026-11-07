# CPU interpreter status

C3350EMU now contains a small, memory-backed ARM-state interpreter. This is the first step toward executing code, not proof that the target's exact CPU core or reset state has been identified.

## Implemented instruction groups

- ARM condition-code evaluation (EQ through AL, with NV not executed).
- Data-processing operations: AND, EOR, SUB, RSB, ADD, TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN.
- Immediate operands with rotated 8-bit constants.
- Register operands with immediate shifts (LSL, LSR, ASR, ROR).
- B and BL branches.
- Basic single data transfers: LDR, STR, LDRB, STRB, immediate offsets, and unshifted register offsets.
- NZCV updates for the implemented arithmetic/test operations.

## Deliberate limitations

- ARM state only. Thumb/Thumb-2 is not implemented.
- No exception entry/return, banked registers, SPSR handling, CP15, MMU, caches, interrupts, timers, or cycle timing.
- No multiply, block transfer, halfword/signed transfer, SWI, coprocessor, or DSP instructions.
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
