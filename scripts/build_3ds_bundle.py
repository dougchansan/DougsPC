#!/usr/bin/env python3
"""Build an SD-card-ready LegacyBox3DS batch bundle."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def build_bundle(source: Path, destination: Path, start_box: int = 0) -> dict:
    report_path = source / "batch-report.json"
    report = json.loads(report_path.read_text(encoding="utf-8"))

    if not report.get("allLegal"):
        raise ValueError("batch-report.json is not fully legal")
    if report.get("failed") != 0:
        raise ValueError("batch contains failed legality entries")

    results = report["results"]
    if not results:
        raise ValueError("batch contains no entries")

    destination.mkdir(parents=True, exist_ok=True)

    rows: list[tuple[str, str, int, int]] = []
    digest_material: list[str] = []

    for index, result in enumerate(results):
        if not result.get("legal"):
            raise ValueError(f"{result.get('name', index)} is not PKHeX-legal")

        filename = result["file"]
        src = source / filename
        if not src.is_file():
            raise FileNotFoundError(src)

        size = src.stat().st_size
        if size not in (232, 260):
            raise ValueError(f"{filename}: expected PK7 size 232 or 260, got {size}")

        digest = sha256_file(src)
        expected = result.get("sha256")
        if expected and digest != expected:
            raise ValueError(f"{filename}: report SHA-256 mismatch")

        box = start_box + index // 30
        slot = index % 30

        shutil.copy2(src, destination / filename)
        rows.append((filename, digest, box, slot))
        digest_material.append(f"{filename}:{digest}:{box}:{slot}")

    batch_id = hashlib.sha256("\n".join(digest_material).encode()).hexdigest()

    lines = [f"LBX1\t{batch_id}\t{len(rows)}\t{start_box}"]
    lines.extend(
        f"{filename}\t{digest}\t{box}\t{slot}"
        for filename, digest, box, slot in rows
    )
    (destination / "batch.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8")

    audit = {
        "schema": "LBX1",
        "batch_id": batch_id,
        "count": len(rows),
        "start_box_index": start_box,
        "start_box_display": start_box + 1,
        "end_box_index": rows[-1][2],
        "end_box_display": rows[-1][2] + 1,
        "source_manifest": report.get("manifest"),
        "trainer": report.get("trainer"),
        "entries": [
            {
                "file": filename,
                "sha256": digest,
                "box_index": box,
                "box_display": box + 1,
                "slot_index": slot,
                "slot_display": slot + 1,
            }
            for filename, digest, box, slot in rows
        ],
    }
    (destination / "bundle-audit.json").write_text(
        json.dumps(audit, indent=2) + "\n", encoding="utf-8"
    )
    return audit


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="directory containing batch-report.json and PK7 files")
    parser.add_argument("destination", type=Path, help="output directory (copy to /3ds/LegacyBox/inbox/current)")
    parser.add_argument("--start-box", type=int, default=0, help="zero-based target box index")
    args = parser.parse_args()

    audit = build_bundle(args.source, args.destination, args.start_box)
    print(json.dumps(audit, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
