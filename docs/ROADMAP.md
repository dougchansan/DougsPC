# Roadmap

## M0 — Core bootstrap

- [x] Manifest schema
- [x] Structural validation
- [x] Deterministic plan IDs
- [x] Explicit unverified legality state
- [x] CLI
- [x] Tests/CI
- [ ] Select legality backend licensing strategy

## M0.5 — Heritage Dex provenance

- [x] Default trainer profile: Doug
- [x] Per-entry player OT vs fixed-event OT policy
- [x] Cross-generation Gen I–VII provenance smoke manifest
- [x] Build #001–#807 curation index with debut-generation/source pools
- [ ] Assign a reviewed encounter template to every #001–#807 entry
- [ ] Add generation-specific Trainer ID/SID policy
- [x] Add fixed-event identity model and fail-closed validation
- [ ] Populate the fixed-event exception table from verified distribution templates

## M0.75 — Source completeness

- [x] Main-series Gen I–VII source families
- [x] Colosseum/XD native + Shadow/Purified model
- [ ] Pokémon Box Ruby & Sapphire bonus Eggs
- [ ] Colosseum Bonus Disc: WISHMKR Jirachi / Ageto Celebi
- [ ] Pokémon Channel Jirachi
- [ ] Japanese Colosseum e-Reader Shadow Togepi/Mareep/Scizor
- [ ] Pokémon Ranger Manaphy Egg lineages
- [ ] My Pokémon Ranch Mew / Phione
- [ ] Pokémon Battle Revolution gifts
- [ ] Pokéwalker course/special-move provenance
- [ ] Pokémon Dream World / PGL Hidden Ability origins
- [ ] Pokémon Dream Radar origins and badge-dependent levels
- [ ] Gen VI/VII special demos / Bank distributions / QR or other game-based gifts
- [ ] Determine which side-game source distinctions survive in PKM data vs lineage-only metadata

## M1 — Real Gen VII legality proof

- [ ] Resolve smoke-set encounter templates against a real encounter database
- [ ] Generate native PK7 candidates
- [ ] Run a real legality analysis for every candidate
- [ ] Emit machine-readable audit reports
- [ ] Add golden-file tests for PK7 serialization
- [ ] Expand smoke set to 25 representative cases:
  - wild
  - starter/gift
  - evolution
  - trade evolution
  - stone/item evolution
  - legendary
  - version exclusive
  - fixed event
  - mythical distribution
  - alternate form

**Exit criterion:** every candidate has a reproducible template and a real legality `PASSED` result; nothing is accepted merely because it parses.

## M2 — USUM save integration

- [ ] Read a user-owned USUM save
- [ ] Create immutable backup + SHA-256
- [ ] Enumerate PC slots
- [ ] Write validated PK7 candidates
- [ ] Re-open and verify save
- [ ] Refuse write on failed/unverified legality

**Exit criterion:** a small generated set can be written to a copied save and re-read byte-for-byte as expected.

## M3 — Living Dex #001–#807

- [ ] Build a reviewed encounter-template manifest
- [ ] Handle event-only/mythical species separately
- [ ] Model forms and Bank-storage exclusions
- [ ] Produce deterministic audit manifest
- [ ] Generate box layout
- [ ] Validate all entries

## M4 — 3DS bridge

- [ ] Homebrew network service
- [ ] Bonjour/mDNS discovery
- [ ] Pairing/authentication
- [ ] Cartridge/save inventory
- [ ] Backup/restore
- [ ] Atomic write + verification

## M5 — iOS LegacyBox

- [ ] Vault UI
- [ ] Collection browser
- [ ] Wireless send/receive
- [ ] Original/derived lineage view
- [ ] Migration checklist

## M6 — Migration workflows

- [ ] Bank-era official transfer checklist while available
- [ ] Preserve post-transfer local audit metadata
- [ ] Keep modern-game export as a separate, validated adapter
