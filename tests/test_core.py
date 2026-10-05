from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from legacybox.manifest import ManifestError, load_manifest
from legacybox.models import VerificationStatus
from legacybox.pipeline import audit_manifest, generation_plan


ROOT = Path(__file__).resolve().parents[1]


class LegacyBoxCoreTests(unittest.TestCase):
    def test_smoke_manifest_loads(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "gen7-smoke.json")
        self.assertEqual(manifest.target_generation, 7)
        self.assertEqual(len(manifest.entries), 6)

    def test_plan_is_deterministic(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "gen7-smoke.json")
        first = generation_plan(manifest)
        second = generation_plan(manifest)
        self.assertEqual(first, second)
        self.assertEqual(len(first[0]["plan_id"]), 16)

    def test_unconfigured_backend_never_claims_legality(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "gen7-smoke.json")
        results = audit_manifest(manifest)
        self.assertTrue(
            all(
                result.legality.status is VerificationStatus.UNVERIFIED
                for result in results
            )
        )

    def test_duplicate_dex_form_is_rejected(self) -> None:
        payload = {
            "name": "duplicate",
            "target_generation": 7,
            "entries": [
                {
                    "national_dex": 1,
                    "species": "Bulbasaur",
                    "target_level": 5,
                    "template": {
                        "template_id": "a",
                        "source_game": "X",
                        "generation": 6,
                        "method": "gift"
                    }
                },
                {
                    "national_dex": 1,
                    "species": "Bulbasaur",
                    "target_level": 6,
                    "template": {
                        "template_id": "b",
                        "source_game": "Y",
                        "generation": 6,
                        "method": "gift"
                    }
                }
            ]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ManifestError):
                load_manifest(path)


if __name__ == "__main__":
    unittest.main()
