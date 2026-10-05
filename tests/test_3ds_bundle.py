from __future__ import annotations

import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "build_3ds_bundle.py"


def load_builder():
    spec = importlib.util.spec_from_file_location("build_3ds_bundle", SCRIPT)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BundleBuilderTests(unittest.TestCase):
    def test_builds_deterministic_lbx1_manifest(self) -> None:
        builder = load_builder()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "source"
            out = root / "out"
            source.mkdir()

            results = []
            for index, name in enumerate(("Bulbasaur", "Ivysaur", "Venusaur"), start=1):
                filename = f"{index:03d}_{name}.pk7"
                data = bytes([index]) * 232
                (source / filename).write_bytes(data)
                results.append(
                    {
                        "species": index,
                        "name": name,
                        "legal": True,
                        "file": filename,
                        "sha256": hashlib.sha256(data).hexdigest(),
                    }
                )

            report = {
                "manifest": "heritage-test.json",
                "trainer": "Doug",
                "total": 3,
                "passed": 3,
                "failed": 0,
                "allLegal": True,
                "results": results,
            }
            (source / "batch-report.json").write_text(
                json.dumps(report), encoding="utf-8"
            )

            audit = builder.build_bundle(source, out, start_box=2)
            self.assertEqual(audit["count"], 3)
            self.assertEqual(audit["start_box_index"], 2)
            self.assertEqual(audit["start_box_display"], 3)

            lines = (out / "batch.tsv").read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(lines), 4)
            self.assertTrue(lines[0].startswith("LBX1\t"))
            self.assertTrue(lines[1].endswith("\t2\t0"))
            self.assertTrue(lines[2].endswith("\t2\t1"))
            self.assertTrue(lines[3].endswith("\t2\t2"))

            for result in results:
                self.assertTrue((out / result["file"]).is_file())

    def test_refuses_nonlegal_batch(self) -> None:
        builder = load_builder()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "source"
            source.mkdir()
            (source / "batch-report.json").write_text(
                json.dumps(
                    {
                        "allLegal": False,
                        "failed": 1,
                        "results": [],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaises(ValueError):
                builder.build_bundle(source, root / "out")


if __name__ == "__main__":
    unittest.main()
