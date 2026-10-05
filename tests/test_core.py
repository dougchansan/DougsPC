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

    def test_heritage_manifest_spans_generations_one_through_seven(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        self.assertEqual(manifest.trainer.name, "Doug")
        self.assertEqual(
            {entry.template.generation for entry in manifest.entries},
            {1, 2, 3, 4, 5, 6, 7},
        )
        self.assertTrue(all(entry.event_identity is None for entry in manifest.entries))
        plan = generation_plan(manifest)
        self.assertTrue(all(item["effective_ot"] == "Doug" for item in plan))

    def test_fixed_event_requires_fixed_ot(self) -> None:
        payload = {
            "name": "event",
            "target_generation": 7,
            "trainer": {"name": "Doug"},
            "entries": [
                {
                    "national_dex": 807,
                    "species": "Zeraora",
                    "target_level": 50,
                    "ot_policy": "fixed_event",
                    "template": {
                        "template_id": "event-placeholder",
                        "source_game": "Ultra Sun/Ultra Moon",
                        "generation": 7,
                        "method": "event",
                        "origin_kind": "fixed_event"
                    }
                }
            ]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ManifestError):
                load_manifest(path)

    def test_fixed_event_ot_is_never_replaced_by_trainer(self) -> None:
        payload = {
            "name": "fixed event",
            "target_generation": 7,
            "trainer": {"name": "Doug"},
            "entries": [
                {
                    "national_dex": 807,
                    "species": "Zeraora",
                    "target_level": 50,
                    "ot_policy": "fixed_event",
                    "event_identity": {
                        "ot": "EVENT-OT",
                        "tid": "000000",
                        "language": "ENG",
                        "distribution_id": "synthetic-test-only",
                        "fateful_encounter": True
                    },
                    "template": {
                        "template_id": "synthetic-event-test",
                        "source_game": "Ultra Sun/Ultra Moon",
                        "generation": 7,
                        "method": "event",
                        "origin_kind": "fixed_event",
                        "source_platform": "Nintendo 3DS",
                        "origin_region": "Alola"
                    }
                }
            ]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            manifest = load_manifest(path)
            item = generation_plan(manifest)[0]
            self.assertEqual(item["effective_ot"], "EVENT-OT")
            self.assertNotEqual(item["effective_ot"], "Doug")

    def test_heritage_index_has_807_unique_species(self) -> None:
        payload = json.loads(
            (ROOT / "data" / "heritage-index.json").read_text(encoding="utf-8")
        )
        entries = payload["entries"]
        self.assertEqual(len(entries), 807)
        self.assertEqual({e["national_dex"] for e in entries}, set(range(1, 808)))
        self.assertEqual(payload["trainer"]["name"], "Doug")
        self.assertTrue(
            all(e["curation_status"] == "pending_encounter_template" for e in entries)
        )

    def test_heritage_templates_are_cross_checked(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        for entry in manifest.entries:
            self.assertEqual(entry.template.verification_status, "cross_checked")
            sources = {ref.source for ref in entry.template.references}
            self.assertIn("Bulbapedia", sources)
            self.assertIn("Serebii", sources)

    def test_corrected_sandile_desert_resort_level(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        sandile = next(e for e in manifest.entries if e.national_dex == 551)
        self.assertEqual((sandile.template.level_min, sandile.template.level_max), (19, 22))
        self.assertEqual(sandile.target_level, 19)

    def test_vc_pokerus_must_be_post_transporter(self) -> None:
        payload = {
            "name": "bad vc pokerus",
            "target_generation": 7,
            "trainer": {"name": "Doug"},
            "entries": [{
                "national_dex": 25,
                "species": "Pikachu",
                "target_level": 5,
                "traits": {
                    "pokerus": "active",
                    "pokerus_acquired_generation": 2
                },
                "template": {
                    "template_id": "vc-yellow-starter-pikachu",
                    "source_game": "Yellow (Virtual Console)",
                    "generation": 1,
                    "method": "starter",
                    "origin_kind": "virtual_console",
                    "source_platform": "Nintendo 3DS Virtual Console"
                }
            }]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ManifestError):
                load_manifest(path)

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
