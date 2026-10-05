from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Any


class VerificationStatus(str, Enum):
    PASSED = "PASSED"
    FAILED = "FAILED"
    UNVERIFIED = "UNVERIFIED"


@dataclass(frozen=True)
class EncounterTemplateRef:
    template_id: str
    source_game: str
    generation: int
    method: str

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "EncounterTemplateRef":
        return cls(
            template_id=str(value["template_id"]).strip(),
            source_game=str(value["source_game"]).strip(),
            generation=int(value["generation"]),
            method=str(value["method"]).strip(),
        )


@dataclass(frozen=True)
class DexEntry:
    national_dex: int
    species: str
    form: str
    target_level: int
    template: EncounterTemplateRef
    notes: str = ""

    @property
    def key(self) -> str:
        form = "" if self.form == "Normal" else f"-{self.form}"
        return f"{self.national_dex:03d}-{self.species}{form}"

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "DexEntry":
        return cls(
            national_dex=int(value["national_dex"]),
            species=str(value["species"]).strip(),
            form=str(value.get("form", "Normal")).strip() or "Normal",
            target_level=int(value["target_level"]),
            template=EncounterTemplateRef.from_dict(value["template"]),
            notes=str(value.get("notes", "")).strip(),
        )


@dataclass(frozen=True)
class LivingDexManifest:
    name: str
    target_generation: int
    entries: tuple[DexEntry, ...]

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "LivingDexManifest":
        return cls(
            name=str(value["name"]).strip(),
            target_generation=int(value["target_generation"]),
            entries=tuple(DexEntry.from_dict(item) for item in value["entries"]),
        )


@dataclass(frozen=True)
class LegalityResult:
    entry_key: str
    status: VerificationStatus
    backend: str
    messages: tuple[str, ...] = ()


@dataclass(frozen=True)
class AuditEntryResult:
    entry_key: str
    structural_status: VerificationStatus
    legality: LegalityResult
    structural_messages: tuple[str, ...] = ()
