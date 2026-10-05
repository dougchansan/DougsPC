from __future__ import annotations

import json
from pathlib import Path

from .models import (
    LivingDexManifest,
    OTPolicy,
    OriginKind,
    PokerusStatus,
)


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
    if not manifest.trainer.name:
        problems.append("trainer name must not be empty")
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
        if len(entry.target_moves) > 4:
            problems.append(f"{entry.key}: target_moves cannot exceed four moves")

        template = entry.template
        if not template.template_id:
            problems.append(f"{entry.key}: template_id must not be empty")
        if not template.source_game:
            problems.append(f"{entry.key}: source_game must not be empty")
        if not 1 <= template.generation <= manifest.target_generation:
            problems.append(
                f"{entry.key}: template generation must be between 1 and target_generation"
            )
        if not template.method:
            problems.append(f"{entry.key}: encounter method must not be empty")
        if len(template.encounter_moves) > 4:
            problems.append(f"{entry.key}: encounter_moves cannot exceed four moves")

        if template.level_min is not None and not 1 <= template.level_min <= 100:
            problems.append(f"{entry.key}: level_min must be between 1 and 100")
        if template.level_max is not None and not 1 <= template.level_max <= 100:
            problems.append(f"{entry.key}: level_max must be between 1 and 100")
        if (
            template.level_min is not None
            and template.level_max is not None
            and template.level_min > template.level_max
        ):
            problems.append(f"{entry.key}: level_min cannot exceed level_max")
        if (
            template.level_min is not None
            and entry.target_level < template.level_min
        ):
            problems.append(
                f"{entry.key}: target_level cannot be below encounter level"
            )

        if template.verification_status == "cross_checked":
            sources = {ref.source.casefold() for ref in template.references}
            if not any("bulbapedia" in source for source in sources):
                problems.append(
                    f"{entry.key}: cross_checked template requires Bulbapedia reference"
                )
            if not any("serebii" in source for source in sources):
                problems.append(
                    f"{entry.key}: cross_checked template requires Serebii reference"
                )

        if entry.ot_policy is OTPolicy.FIXED_EVENT:
            if entry.event_identity is None:
                problems.append(
                    f"{entry.key}: fixed_event OT policy requires event_identity"
                )
            elif not entry.event_identity.ot:
                problems.append(
                    f"{entry.key}: fixed_event event_identity requires OT"
                )
            if template.origin_kind is not OriginKind.FIXED_EVENT:
                problems.append(
                    f"{entry.key}: fixed_event OT policy requires fixed_event origin_kind"
                )
        elif entry.event_identity is not None:
            problems.append(
                f"{entry.key}: trainer OT policy must not specify event_identity"
            )

        if (
            template.origin_kind is OriginKind.VIRTUAL_CONSOLE
            and "virtual console" not in template.source_platform.casefold()
        ):
            problems.append(
                f"{entry.key}: virtual_console origin must identify a Virtual Console platform"
            )

        traits = entry.traits
        if traits.pokerus is PokerusStatus.NONE:
            if traits.pokerus_acquired_generation is not None:
                problems.append(
                    f"{entry.key}: pokerus acquisition generation requires active/cured status"
                )
        else:
            if traits.pokerus_acquired_generation is None:
                problems.append(
                    f"{entry.key}: active/cured pokerus requires acquisition generation"
                )
            elif not 2 <= traits.pokerus_acquired_generation <= manifest.target_generation:
                problems.append(
                    f"{entry.key}: pokerus acquisition generation must be 2..target_generation"
                )
            elif (
                template.origin_kind is OriginKind.VIRTUAL_CONSOLE
                and traits.pokerus_acquired_generation < 7
            ):
                problems.append(
                    f"{entry.key}: VC Pokérus is stripped by Transporter; final Pokérus must be acquired in Gen VII or later"
                )

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
