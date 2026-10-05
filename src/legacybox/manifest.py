from __future__ import annotations

import json
from pathlib import Path

from .models import LivingDexManifest


class ManifestError(ValueError):
    pass


def load_manifest(path: str | Path) -> LivingDexManifest:
    source = Path(path)
    try:
        raw = json.loads(source.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ManifestError(f"Unable to read manifest {source}: {exc}") from exc

    try:
        manifest = LivingDexManifest.from_dict(raw)
    except (KeyError, TypeError, ValueError) as exc:
        raise ManifestError(f"Invalid manifest schema: {exc}") from exc

    problems = validate_manifest_structure(manifest)
    if problems:
        raise ManifestError("; ".join(problems))
    return manifest


def validate_manifest_structure(manifest: LivingDexManifest) -> list[str]:
    problems: list[str] = []

    if not manifest.name:
        problems.append("manifest name must not be empty")
    if not 1 <= manifest.target_generation <= 9:
        problems.append("target_generation must be between 1 and 9")
    if not manifest.entries:
        problems.append("manifest must contain at least one entry")

    seen_dex: set[tuple[int, str]] = set()
    seen_keys: set[str] = set()

    for entry in manifest.entries:
        if not 1 <= entry.national_dex <= 1025:
            problems.append(f"{entry.key}: national_dex out of supported range")
        if not entry.species:
            problems.append(f"{entry.key}: species must not be empty")
        if not 1 <= entry.target_level <= 100:
            problems.append(f"{entry.key}: target_level must be between 1 and 100")
        if not entry.template.template_id:
            problems.append(f"{entry.key}: template_id must not be empty")
        if not entry.template.source_game:
            problems.append(f"{entry.key}: source_game must not be empty")
        if not 1 <= entry.template.generation <= manifest.target_generation:
            problems.append(
                f"{entry.key}: template generation must be between 1 and target_generation"
            )
        if not entry.template.method:
            problems.append(f"{entry.key}: encounter method must not be empty")

        dex_form = (entry.national_dex, entry.form.casefold())
        if dex_form in seen_dex:
            problems.append(
                f"{entry.key}: duplicate national dex/form combination {dex_form}"
            )
        seen_dex.add(dex_form)

        if entry.key in seen_keys:
            problems.append(f"{entry.key}: duplicate entry key")
        seen_keys.add(entry.key)

    return problems
