# LegacyBox

**Preserve. Validate. Migrate.**

LegacyBox is a personal preservation and migration toolkit for Pokémon save data. The first target is a Bank-era living-dex workflow with an auditable path from encounter template selection to a validated Gen VII candidate.

## Design rules

- Preserve original PKM bytes before any transformation.
- Never report a candidate as legal until a real legality backend verifies it.
- Keep generation, legality, save writing, 3DS networking, and iOS UI as separate modules.
- Do not fabricate Pokémon HOME or Pokémon Bank server metadata.
- Treat generated Pokémon as generated data even when they satisfy legality constraints.

## Bootstrap milestone

This branch implements the first testable core:

- JSON living-dex manifests;
- structural validation;
- deterministic generation-plan IDs;
- a pluggable legality-backend contract;
- explicit `UNVERIFIED` status when no legality engine is configured;
- CLI audit/plan commands;
- smoke tests and CI.

The sample manifest is intentionally small. It proves the pipeline before we scale to National Dex #001–#807.

## Run locally

```bash
python -m pip install -e .
legacybox plan manifests/gen7-smoke.json
legacybox audit manifests/gen7-smoke.json
```

Require a real legality decision in automation:

```bash
legacybox audit manifests/gen7-smoke.json --require-legality
```

Until a legality backend is implemented, that final command fails by design rather than pretending the entries are legal.

## Planned adapters

- encounter/template database;
- PK7 candidate generator;
- legality validator;
- USUM save writer with mandatory backup;
- 3DS homebrew bridge;
- iOS vault/client.

See `docs/ARCHITECTURE.md` and `docs/ROADMAP.md`.

## Licensing note

PKHeX.Core is GPL-3.0-or-later. LegacyBox currently keeps its legality interface backend-neutral so we can make an explicit licensing decision before directly linking or incorporating GPL code.

## Disclaimer

LegacyBox is an independent fan-made preservation tool and is not affiliated with Nintendo, The Pokémon Company, or Game Freak. Users are responsible for backing up their own save data and complying with applicable game and service terms.


## Heritage Dex

Heritage Dex mode targets National Dex #001–#807 with visible cross-generation provenance. The default player OT is **Doug**. Fixed distributions are never rewritten to Doug; their event identity is preserved as immutable manifest data.

The full 807-entry curation inventory lives in `data/heritage-index.json`. Entries remain pending until a real encounter template and legality result are attached.


## Archive Mode

LegacyBox can grow beyond one-per-species Heritage Dex generation into an exhaustive **Archive Mode**. Archive Mode tracks materially distinct legal variants by origin game/generation, ability availability, regional/form differences, event distribution, shiny state, special move/ribbon provenance, and GameCube Colosseum/XD Shadow->Purified lineage.

The goal is exhaustive *obtainable provenance*, not an invalid Cartesian product of every field.


## Bank / HOME provenance

LegacyBox does not equate “PKHeX-legal,” “historically legitimate,” and “HOME accepted.”

The project records origin evidence, local legality, official Bank-transfer evidence, and HOME tracker observation as separate trust axes. HOME tracker values are never fabricated or treated as locally verifiable server credentials. See `docs/HOME_PROVENANCE.md`.
