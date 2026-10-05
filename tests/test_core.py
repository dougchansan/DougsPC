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
        statuses = [e["curation_status"] for e in entries]
        self.assertEqual(
            statuses.count("cross_checked_template"),
            payload["progress"]["cross_checked_templates"],
        )
        self.assertEqual(
            statuses.count("pending_encounter_template"),
            payload["progress"]["pending_encounter_templates"],
        )
        self.assertEqual(
            statuses.count("pkhex_passed"),
            payload["progress"]["pkhex_legalized"],
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

    def test_competitive_profiles_have_valid_evs_and_ivs(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        for entry in manifest.entries:
            comp = entry.competitive
            self.assertEqual(comp.verification_status, "smogon_checked")
            self.assertLessEqual(sum(comp.evs.values()), 510)
            self.assertTrue(all(0 <= v <= 252 for v in comp.evs.values()))
            self.assertTrue(all(0 <= v <= 31 for v in comp.ivs.values()))
            self.assertLessEqual(len(comp.moves), 4)

    def test_provenance_can_override_competitive_ability(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        riolu = next(e for e in manifest.entries if e.national_dex == 447)
        fennekin = next(e for e in manifest.entries if e.national_dex == 653)
        self.assertEqual(riolu.competitive.ability, "Inner Focus")
        self.assertEqual(riolu.competitive.application_policy, "preserve_origin_over_competitive")
        self.assertEqual(fennekin.competitive.ability, "Blaze")
        self.assertEqual(fennekin.competitive.application_policy, "preserve_origin_over_competitive")

    def test_hidden_power_iv_patterns_are_explicit(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-smoke.json")
        pikachu = next(e for e in manifest.entries if e.national_dex == 25)
        cyndaquil = next(e for e in manifest.entries if e.national_dex == 155)
        self.assertEqual(
            pikachu.competitive.ivs,
            {"HP": 31, "Atk": 0, "Def": 30, "SpA": 31, "SpD": 31, "Spe": 31},
        )
        self.assertEqual(
            cyndaquil.competitive.ivs,
            {"HP": 31, "Atk": 0, "Def": 31, "SpA": 30, "SpD": 31, "Spe": 31},
        )

    def test_kanto_001_009_batch(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "heritage-001-009.json")
        self.assertEqual([e.national_dex for e in manifest.entries], list(range(1, 10)))
        self.assertEqual(manifest.trainer.name, "Doug")

        venusaur = next(e for e in manifest.entries if e.national_dex == 3)
        charizard = next(e for e in manifest.entries if e.national_dex == 6)
        blastoise = next(e for e in manifest.entries if e.national_dex == 9)

        self.assertEqual(venusaur.template.origin_species, "Bulbasaur")
        self.assertIn("Venusaur at Lv.32", venusaur.template.evolution_path)
        self.assertEqual(charizard.template.origin_species, "Charmander")
        self.assertIn("Charizard at Lv.36", charizard.template.evolution_path)
        self.assertEqual(blastoise.template.origin_species, "Squirtle")
        self.assertIn("Blastoise at Lv.36", blastoise.template.evolution_path)

        self.assertEqual(venusaur.competitive.ability, "Chlorophyll")
        self.assertEqual(charizard.competitive.ability, "Solar Power")
        self.assertEqual(blastoise.competitive.ability, "Rain Dish")
        self.assertTrue(all(e.template.verification_status == "cross_checked" for e in manifest.entries))
        self.assertTrue(all(e.competitive.verification_status == "smogon_checked" for e in manifest.entries))

    def test_shadow_origin_requires_purification_and_ribbon(self) -> None:
        payload = {
            "name": "shadow",
            "target_generation": 7,
            "trainer": {"name": "Doug"},
            "entries": [{
                "national_dex": 197,
                "species": "Umbreon",
                "target_level": 26,
                "template": {
                    "template_id": "colo-shadow-test",
                    "source_game": "Pokémon Colosseum",
                    "generation": 3,
                    "method": "shadow-snag",
                    "origin_kind": "shadow_purified",
                    "source_platform": "Nintendo GameCube",
                    "shadow": {
                        "game": "Pokémon Colosseum",
                        "purified": False,
                        "national_ribbon": False
                    }
                }
            }]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaises(ManifestError):
                load_manifest(path)

    def test_valid_shadow_origin_requires_national_ribbon(self) -> None:
        payload = {
            "name": "shadow",
            "target_generation": 7,
            "trainer": {"name": "Doug"},
            "entries": [{
                "national_dex": 197,
                "species": "Umbreon",
                "target_level": 26,
                "template": {
                    "template_id": "colo-purified-test",
                    "source_game": "Pokémon Colosseum",
                    "generation": 3,
                    "method": "shadow-snag",
                    "origin_kind": "shadow_purified",
                    "source_platform": "Nintendo GameCube",
                    "shadow": {
                        "game": "Pokémon Colosseum",
                        "purified": True,
                        "national_ribbon": True
                    }
                }
            }]
        }
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "manifest.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            manifest = load_manifest(path)
            self.assertTrue(manifest.entries[0].template.shadow.national_ribbon)

    def test_gamecube_smoke_preserves_shadow_provenance(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "gamecube-smoke.json")
        self.assertEqual(len(manifest.entries), 3)

        espeon = next(e for e in manifest.entries if e.national_dex == 196)
        makuhita = next(e for e in manifest.entries if e.national_dex == 296)
        teddiursa = next(e for e in manifest.entries if e.national_dex == 216)

        self.assertIsNone(espeon.template.shadow)
        self.assertEqual(makuhita.template.origin_kind.value, "shadow_purified")
        self.assertTrue(makuhita.template.shadow.purified)
        self.assertTrue(makuhita.template.shadow.national_ribbon)
        self.assertEqual(teddiursa.template.shadow.purification_move, "Refresh")

    def test_shadow_moves_are_not_target_export_moves(self) -> None:
        manifest = load_manifest(ROOT / "manifests" / "gamecube-smoke.json")
        for entry in manifest.entries:
            if entry.template.shadow is None:
                continue
            self.assertTrue(all(not move.startswith("Shadow ") for move in entry.target_moves))

    def test_side_source_registry_has_full_pre_switch_coverage(self) -> None:
        payload = json.loads(
            (ROOT / "data" / "side-sources.json").read_text(encoding="utf-8")
        )
        ids = {source["id"] for source in payload["sources"]}
        required = {
            "gen3.pokemon_box_bonus_eggs",
            "gen3.wishmkr_jirachi",
            "gen3.ageto_celebi",
            "gen3.channel_jirachi",
            "gen3.colosseum_jp_ereader",
            "gen4.ranger_manaphy_egg",
            "gen4.my_pokemon_ranch",
            "gen4.battle_revolution",
            "gen4.pokewalker",
            "gen5.dream_world",
            "gen5.dream_radar",
            "gen6.oras_special_demo",
            "gen6.pokemon_bank_distributions",
            "gen7.sm_special_demo_ash_greninja",
            "gen7.pokemon_bank_distributions",
            "gen7.qr_game_based_gifts",
            "gen7.island_scan",
        }
        self.assertEqual(required - ids, set())
        self.assertEqual(len(ids), 17)

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
