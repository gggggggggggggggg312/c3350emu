# Modifying the actual PSI firmware

Use `tools/scan_psi.ps1` on the Windows PC that contains the Samsung firmware. It scans the known candidate PSI entry area and prints ARM instructions and branch destinations. This avoids manually opening the binary in a hex editor.

When we have verified which instruction initializes an unsupported phone peripheral, add a patch to `tools/psi_emulator_patches.json`:

```json
{
  "patches": [
    {
      "file_offset": "0x1234",
      "expected_hex": "old bytes in file order",
      "replacement_hex": "new bytes in file order",
      "reason": "verified purpose of this change"
    }
  ]
}
```

Then create the emulator-specific image:

```powershell
python .\tools\patch_psi.py "$env:USERPROFILE\Downloads\C3350XXKK1_OXEKK1\C3350XXKK1_BIN_psi.fls" --manifest .\tools\psi_emulator_patches.json --output "$env:USERPROFILE\Downloads\C3350XXKK1_OXEKK1\C3350XXKK1_BIN_psi_EMULATOR.fls"
```

The tool checks every original byte before patching and refuses mismatches. It writes a separate output file and reports both SHA-256 hashes. The original PSI is never overwritten.

**Current state:** no speculative patch is enabled. The known entry at file offset `0xCF8` branches to candidate code at `0xB878`, but we have not yet proven which PSI routine configures the C3350 display or other hardware. Replacing that branch blindly could destroy the boot path. The scan output is the next evidence needed to make the first real firmware patch.
