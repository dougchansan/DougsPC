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

    allowed = {"pending_encounter_template", "cross_checked_template", "pkhex_passed"}
    unknown = [
        (entry["national_dex"], entry["curation_status"])
        for entry in entries
        if entry["curation_status"] not in allowed
    ]
    if unknown:
        raise SystemExit(f"unknown curation statuses: {unknown}")

    verified = [
        entry for entry in entries
        if entry["curation_status"] == "cross_checked_template"
    ]
    pending = [
        entry for entry in entries
        if entry["curation_status"] == "pending_encounter_template"
    ]
    legalized = [
        entry for entry in entries
        if entry["curation_status"] == "pkhex_passed"
    ]

    progress = payload.get("progress", {})
    expected = {
        "total": 807,
        "cross_checked_templates": len(verified),
        "pending_encounter_templates": len(pending),
        "pkhex_legalized": len(legalized),
    }
    if progress != expected:
        raise SystemExit(f"progress block mismatch: expected {expected}, got {progress}")

    print(
        "Heritage Dex index OK: "
        f"{len(verified)} cross-checked, "
        f"{len(legalized)} PKHeX-passed, "
        f"{len(pending)} pending."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
