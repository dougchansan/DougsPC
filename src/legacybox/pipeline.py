from __future__ import annotations

import hashlib
import json
from dataclasses import asdict

from .backend import LegalityBackend, UnconfiguredLegalityBackend
from .manifest import validate_manifest_structure
from .models import (
    AuditEntryResult,
    LivingDexManifest,
    VerificationStatus,
)


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
                "template_id": entry.template.template_id,
                "source_game": entry.template.source_game,
                "generation": entry.template.generation,
                "method": entry.template.method,
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
