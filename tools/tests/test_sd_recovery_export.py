import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zlib

spec = importlib.util.spec_from_file_location("recovery_export", Path(__file__).parents[1] / "sd_recovery_export.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def sample():
    path = b"/infrared/remote/Test.ir".hex().upper()
    payload = b"hello\x00\xff"
    return ("[SD PARTITION] selected=1 start=16 sectors=100000 legacy=1\n"
            "[SD INVENTORY] begin\n"
            "[SD INVENTORY] path_hex=2F696E667261726564 kind=directory size=0 known=1\n"
            f"[SD INVENTORY] path_hex={path} kind=file size=7 known=1\n"
            "[SD INVENTORY] end ok=1 files=1 directories=1 bytes=7 unknown_nonempty=0\n"
            "[SD BACKUP] begin files=1 memory_bytes=1024\n"
            f"[SD BACKUP] file=0 path_hex={path} size=7 crc32={zlib.crc32(payload):08X}\n"
            f"[SD BACKUP] file=0 offset=0 hex={payload.hex().upper()}\n"
            "[SD BACKUP] file=0 end\n"
            "[SD BACKUP] end files=1\n"
            "[SD RECOVERY] NEEDS_HOST_RESTORE files=1; verify durable PC backup\n")


class RecoveryExportTests(unittest.TestCase):
    def test_complete_binary_export_and_readback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log = root / "boot.log"
            log.write_text("\n".join("timestamp " + line for line in sample().splitlines()), encoding="utf-8")
            module.export(log, root / "backup")
            self.assertEqual((root / "backup/0000.bin").read_bytes(), b"hello\x00\xff")
            result = json.loads((root / "backup/manifest.json").read_text())
            self.assertTrue(result["verified"])
            self.assertEqual(result["files"][0]["path"], "infrared/remote/Test.ir")
            self.assertEqual((root / "backup/identity.txt").read_bytes(), b"MEOWKIT SD RECOVERY v1\npartition=1 start=16 sectors=100000\n")
            with self.assertRaises(FileExistsError):
                module.export(log, root / "backup")

    def test_missing_duplicate_reordered_corrupt_and_unsafe_records(self):
        good = sample()
        variants = [
            good + good,
            good.replace("[SD BACKUP] file=0 end\n", ""),
            good.replace("offset=0", "offset=1"),
            good.replace("68656C6C6F00FF", "68656C6C6F00FE"),
            good.replace("unknown_nonempty=0", "unknown_nonempty=1"),
            good.replace("end files=1", "end files=2"),
            good.replace("kind=file size=7", "kind=file size=8"),
            good.replace(b"/infrared/remote/Test.ir".hex().upper(), b"/infrared/../escape".hex().upper()),
            good.replace("NEEDS_HOST_RESTORE", "REFUSED"),
            good.replace("[SD INVENTORY] begin\n", ""),
        ]
        for text in variants:
            with self.subTest(text=text[-80:]), self.assertRaises(ValueError):
                module.decode(text)


if __name__ == "__main__":
    unittest.main()
