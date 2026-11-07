import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from analyze_flashtool_log import parse_log


SAMPLE = """
Start downloading item 'BOOT CORE PSI:psi.fls' from file 'C:\\fw\\psi.fls'.
Sending sec-pack.
  Load region 0
  - Sending data.
Sending end-pack.
Checksum OK (0x0000)
Verify OK
Process time was 717 msec.
Start downloading item 'CODE:CATFISH.fls' from file 'C:\\fw\\BSY.fls'.
Sending sec-pack.
  Load region 0
  - Sending data.
  Load region 1
  - Sending data.
Sending end-pack.
Checksum OK (0x0000)
Verify OK
Process time was 38.6 sec.
"""


class FlashToolLogTests(unittest.TestCase):
    def test_extracts_component_and_region_records(self):
        records = parse_log(SAMPLE)
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0]["kind"], "BOOT CORE PSI")
        self.assertEqual(records[0]["source_file"], "psi.fls")
        self.assertEqual(records[0]["regions"], [0])
        self.assertTrue(records[0]["verified"])
        self.assertEqual(records[1]["kind"], "CODE")
        self.assertEqual(records[1]["source_file"], "BSY.fls")
        self.assertEqual(records[1]["regions"], [0, 1])
        self.assertEqual(records[1]["checksum"], "0X0000")
        self.assertEqual(records[1]["process_time"], "38.6 sec")

    def test_empty_log(self):
        self.assertEqual(parse_log("nothing to parse"), [])


if __name__ == "__main__":
    unittest.main()
