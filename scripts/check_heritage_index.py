#!/usr/bin/env python3
"""Validate the checked-in Heritage Dex curation index."""

from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INDEX = ROOT / "data" / "heritage-index.json"


def main() -> int:
    payload = json.loads(INDEX.read_text(encoding="utf-8"))
    entries = payload["entries"]

    if payload["trainer"]["name"] != "Doug":
        raise SystemExit("Heritage Dex trainer must be Doug")
    if len(entries) != 807:
        raise SystemExit(f"expected 807 entries, found {len(entries)}")

    dex = [entry["national_dex"] for entry in entries]
    if dex != list(range(1, 808)):
        raise SystemExit("National Dex index must be exactly #001 through #807")

    species = [entry["species"] for entry in entries]
    if len(set(species)) != 807:
        raise SystemExit("species names must be unique in the base living dex")

    invalid = [
        entry["national_dex"]
        for entry in entries
        if entry["curation_status"] != "pending_encounter_template"
    ]
    if invalid:
        raise SystemExit(
            "bootstrap index must fail closed; unexpected curated entries: "
            + ", ".join(map(str, invalid))
        )

    print("Heritage Dex index OK: 807/807 entries inventoried; 0 claimed curated.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
