#!/usr/bin/env python3
"""Validate the side-source registry."""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PATH = ROOT / "data" / "side-sources.json"


def main() -> int:
    payload = json.loads(PATH.read_text(encoding="utf-8"))
    sources = payload["sources"]

    ids = [source["id"] for source in sources]
    if len(ids) != len(set(ids)):
        raise SystemExit("side-source IDs must be unique")

    expected = {
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
    }
    missing = expected - set(ids)
    if missing:
        raise SystemExit(f"missing required side-source families: {sorted(missing)}")

    for source in sources:
        if source["status"] not in {"catalogued", "cross_checked", "generated", "pkhex_passed"}:
            raise SystemExit(f"{source['id']}: invalid status")
        if not source.get("transfer_route"):
            raise SystemExit(f"{source['id']}: transfer route is required")
        if not source.get("references"):
            raise SystemExit(f"{source['id']}: at least one evidence reference is required")
        if not source.get("pkhex_support"):
            raise SystemExit(f"{source['id']}: PKHeX support mapping is required")

    box = next(x for x in sources if x["id"] == "gen3.pokemon_box_bonus_eggs")
    if {v["exclusive_move"] for v in box["variants"]} != {"False Swipe", "ExtremeSpeed", "Pay Day", "Surf"}:
        raise SystemExit("Pokémon Box exclusive move set mismatch")

    radar = next(x for x in sources if x["id"] == "gen5.dream_radar")
    rules = " ".join(radar["rules"])
    if "5 to 40" not in rules or "Dream Ball" not in rules:
        raise SystemExit("Dream Radar badge-level/Dream Ball rules missing")

    print(f"Side-source registry OK: {len(sources)} source families catalogued.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
