from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Any


class VerificationStatus(str, Enum):
    PASSED = "PASSED"
    FAILED = "FAILED"
    UNVERIFIED = "UNVERIFIED"


class OTPolicy(str, Enum):
    TRAINER = "trainer"
    FIXED_EVENT = "fixed_event"


class OriginKind(str, Enum):
    NATIVE = "native"
    VIRTUAL_CONSOLE = "virtual_console"
    EVOLUTION = "evolution"
    GIFT = "gift"
    FIXED_EVENT = "fixed_event"


class ShinyPolicy(str, Enum):
    RANDOM_LEGAL = "random_legal"
    FORCE_SHINY = "force_shiny"
    FORCE_NON_SHINY = "force_non_shiny"


class PokerusStatus(str, Enum):
    NONE = "none"
    ACTIVE = "active"
    CURED = "cured"


@dataclass(frozen=True)
class TrainerProfile:
    name: str
    language: str = "ENG"
    id_strategy: str = "deterministic-per-origin"

    @classmethod
    def from_dict(cls, value: dict[str, Any] | None) -> "TrainerProfile":
        value = value or {}
        return cls(
            name=str(value.get("name", "Doug")).strip(),
            language=str(value.get("language", "ENG")).strip().upper(),
            id_strategy=str(
                value.get("id_strategy", "deterministic-per-origin")
            ).strip(),
        )


@dataclass(frozen=True)
class EventIdentity:
    """Distribution-controlled identity that LegacyBox must not overwrite."""

    ot: str
    tid: str | None = None
    sid: str | None = None
    language: str | None = None
    distribution_id: str | None = None
    fateful_encounter: bool | None = None
    locked_fields: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def from_dict(cls, value: dict[str, Any] | None) -> "EventIdentity | None":
        if value is None:
            return None
        return cls(
            ot=str(value["ot"]).strip(),
            tid=None if value.get("tid") is None else str(value["tid"]).strip(),
            sid=None if value.get("sid") is None else str(value["sid"]).strip(),
            language=(
                None
                if value.get("language") is None
                else str(value["language"]).strip().upper()
            ),
            distribution_id=(
                None
                if value.get("distribution_id") is None
                else str(value["distribution_id"]).strip()
            ),
            fateful_encounter=(
                None
                if value.get("fateful_encounter") is None
                else bool(value["fateful_encounter"])
            ),
            locked_fields=dict(value.get("locked_fields", {})),
        )


@dataclass(frozen=True)
class SourceReference:
    source: str
    url: str
    checked_fields: tuple[str, ...] = ()
    checked_date: str = ""

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "SourceReference":
        return cls(
            source=str(value["source"]).strip(),
            url=str(value["url"]).strip(),
            checked_fields=tuple(
                str(field).strip()
                for field in value.get("checked_fields", [])
                if str(field).strip()
            ),
            checked_date=str(value.get("checked_date", "")).strip(),
        )


@dataclass(frozen=True)
class TraitPlan:
    shiny: ShinyPolicy = ShinyPolicy.RANDOM_LEGAL
    shiny_phase: str = "origin"
    pokerus: PokerusStatus = PokerusStatus.NONE
    pokerus_acquired_generation: int | None = None

    @classmethod
    def from_dict(cls, value: dict[str, Any] | None) -> "TraitPlan":
        value = value or {}
        gen = value.get("pokerus_acquired_generation")
        return cls(
            shiny=ShinyPolicy(
                str(value.get("shiny", ShinyPolicy.RANDOM_LEGAL.value))
            ),
            shiny_phase=str(value.get("shiny_phase", "origin")).strip(),
            pokerus=PokerusStatus(
                str(value.get("pokerus", PokerusStatus.NONE.value))
            ),
            pokerus_acquired_generation=None if gen is None else int(gen),
        )


@dataclass(frozen=True)
class EncounterTemplateRef:
    template_id: str
    source_game: str
    generation: int
    method: str
    origin_kind: OriginKind = OriginKind.NATIVE
    source_platform: str = ""
    origin_region: str = ""
    location: str = ""
    level_min: int | None = None
    level_max: int | None = None
    encounter_moves: tuple[str, ...] = ()
    ability: str = ""
    held_item: str = ""
    nature_policy: str = "random"
    verification_status: str = "unverified"
    references: tuple[SourceReference, ...] = ()

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "EncounterTemplateRef":
        level_min = value.get("level_min")
        level_max = value.get("level_max")
        return cls(
            template_id=str(value["template_id"]).strip(),
            source_game=str(value["source_game"]).strip(),
            generation=int(value["generation"]),
            method=str(value["method"]).strip(),
            origin_kind=OriginKind(
                str(value.get("origin_kind", OriginKind.NATIVE.value))
            ),
            source_platform=str(value.get("source_platform", "")).strip(),
            origin_region=str(value.get("origin_region", "")).strip(),
            location=str(value.get("location", "")).strip(),
            level_min=None if level_min is None else int(level_min),
            level_max=None if level_max is None else int(level_max),
            encounter_moves=tuple(
                str(move).strip()
                for move in value.get("encounter_moves", [])
                if str(move).strip()
            ),
            ability=str(value.get("ability", "")).strip(),
            held_item=str(value.get("held_item", "")).strip(),
            nature_policy=str(value.get("nature_policy", "random")).strip(),
            verification_status=str(
                value.get("verification_status", "unverified")
            ).strip(),
            references=tuple(
                SourceReference.from_dict(ref) for ref in value.get("references", [])
            ),
        )


@dataclass(frozen=True)
class DexEntry:
    national_dex: int
    species: str
    form: str
    target_level: int
    template: EncounterTemplateRef
    ot_policy: OTPolicy = OTPolicy.TRAINER
    event_identity: EventIdentity | None = None
    migration_route: tuple[str, ...] = ()
    target_moves: tuple[str, ...] = ()
    traits: TraitPlan = TraitPlan()
    notes: str = ""

    @property
    def key(self) -> str:
        form = "" if self.form == "Normal" else f"-{self.form}"
        return f"{self.national_dex:03d}-{self.species}{form}"

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "DexEntry":
        policy = OTPolicy(str(value.get("ot_policy", OTPolicy.TRAINER.value)))
        return cls(
            national_dex=int(value["national_dex"]),
            species=str(value["species"]).strip(),
            form=str(value.get("form", "Normal")).strip() or "Normal",
            target_level=int(value["target_level"]),
            template=EncounterTemplateRef.from_dict(value["template"]),
            ot_policy=policy,
            event_identity=EventIdentity.from_dict(value.get("event_identity")),
            migration_route=tuple(
                str(stage).strip()
                for stage in value.get("migration_route", [])
                if str(stage).strip()
            ),
            target_moves=tuple(
                str(move).strip()
                for move in value.get("target_moves", [])
                if str(move).strip()
            ),
            traits=TraitPlan.from_dict(value.get("traits")),
            notes=str(value.get("notes", "")).strip(),
        )


@dataclass(frozen=True)
class LivingDexManifest:
    name: str
    target_generation: int
    trainer: TrainerProfile
    entries: tuple[DexEntry, ...]

    @classmethod
    def from_dict(cls, value: dict[str, Any]) -> "LivingDexManifest":
        return cls(
            name=str(value["name"]).strip(),
            target_generation=int(value["target_generation"]),
            trainer=TrainerProfile.from_dict(value.get("trainer")),
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
