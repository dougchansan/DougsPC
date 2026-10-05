from __future__ import annotations

from typing import Protocol

from .models import DexEntry, LegalityResult, VerificationStatus


class LegalityBackend(Protocol):
    @property
    def name(self) -> str:
        ...

    def validate(self, entry: DexEntry) -> LegalityResult:
        ...


class UnconfiguredLegalityBackend:
    """Fail-safe backend used until a real legality engine is connected."""

    @property
    def name(self) -> str:
        return "unconfigured"

    def validate(self, entry: DexEntry) -> LegalityResult:
        return LegalityResult(
            entry_key=entry.key,
            status=VerificationStatus.UNVERIFIED,
            backend=self.name,
            messages=(
                "No legality backend is configured; this entry has not been proven legal.",
            ),
        )
