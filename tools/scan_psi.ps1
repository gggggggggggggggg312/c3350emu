$psi = "$env:USERPROFILE\Downloads\C3350XXKK1_OXEKK1\C3350XXKK1_BIN_psi.fls"
$tool = Join-Path $PSScriptRoot "patch_psi.py"
python $tool $psi --scan --offset 0xCF8 --count 64 --virtual-base 0xCF8
