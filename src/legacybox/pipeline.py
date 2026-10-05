from __future__ import annotations

import hashlib
import json
from dataclasses import asdict

from .backend import LegalityBackend, UnconfiguredLegalityBackend
from .manifest import validate_manifest_structure
from .models import (
    AuditEntryResult,
    DexEntry,
    LivingDexManifest,
    OTPolicy,
    VerificationStatus,
)


def resolve_effective_ot(manifest: LivingDexManifest, entry: DexEntry) -> str:
    if entry.ot_policy is OTPolicy.FIXED_EVENT:
        if entry.event_identity is None:
            raise ValueError(f"{entry.key}: missing fixed event identity")
        return entry.event_identity.ot
    return manifest.trainer.name


def generation_plan(manifest: LivingDexManifest) -> list[dict[str, object]]:
    plan: list[dict[str, object]] = []
    for index, entry in enumerate(manifest.entries, start=1):
        normalized = json.dumps(asdict(entry), sort_keys=True, separators=(",", ":"))
        plan_id = hashlib.sha256(normalized.encode("utf-8")).hexdigest()[:16]
        plan.append(
            {
                "sequence": index,
                "plan_id": plan_id,
                "entry_key": entry.key,
                "national_dex": entry.national_dex,
                "species": entry.species,
                "form": entry.form,
                "target_level": entry.target_level,
                "target_moves": list(entry.target_moves),
                "traits": {
                    "shiny": entry.traits.shiny.value,
                    "shiny_phase": entry.traits.shiny_phase,
                    "pokerus": entry.traits.pokerus.value,
                    "pokerus_acquired_generation": (
                        entry.traits.pokerus_acquired_generation
                    ),
                },
                "effective_ot": resolve_effective_ot(manifest, entry),
                "ot_policy": entry.ot_policy.value,
                "event_identity": (
                    None if entry.event_identity is None else asdict(entry.event_identity)
                ),
                "template_id": entry.template.template_id,
                "source_game": entry.template.source_game,
                "source_platform": entry.template.source_platform,
                "origin_region": entry.template.origin_region,
                "origin_kind": entry.template.origin_kind.value,
                "generation": entry.template.generation,
                "method": entry.template.method,
                "location": entry.template.location,
                "encounter_level": [
                    entry.template.level_min,
                    entry.template.level_max,
                ],
                "encounter_moves": list(entry.template.encounter_moves),
                "ability": entry.template.ability,
                "held_item": entry.template.held_item,
                "nature_policy": entry.template.nature_policy,
                "verification_status": entry.template.verification_status,
                "references": [asdict(ref) for ref in entry.template.references],
                "migration_route": list(entry.migration_route),
            }
        )
    return plan


def audit_manifest(
    manifest: LivingDexManifest,
    backend: LegalityBackend | None = None,
) -> list[AuditEntryResult]:
    backend = backend or UnconfiguredLegalityBackend()
    global_problems = validate_manifest_structure(manifest)

    results: list[AuditEntryResult] = []
    for entry in manifest.entries:
        entry_prefix = f"{entry.key}:"
        structural_messages = tuple(
            problem for problem in global_problems if problem.startswith(entry_prefix)
        )
        structural_status = (
            VerificationStatus.FAILED
            if structural_messages
            else VerificationStatus.PASSED
        )
        legality = backend.validate(entry)
        results.append(
            AuditEntryResult(
                entry_key=entry.key,
                structural_status=structural_status,
                legality=legality,
                structural_messages=structural_messages,
            )
        )
    return results
