# Heritage Dex provenance policy

LegacyBox Heritage Dex mode preserves visible origin history instead of generating every entry as if it originated in the newest supported game.

## Preferred origin families

| Era | Preferred source titles |
| --- | --- |
| Gen I | Red / Blue / Yellow **Virtual Console** |
| Gen II | Gold / Silver / Crystal **Virtual Console** |
| Gen III | Ruby / Sapphire / Emerald / FireRed / LeafGreen |
| Gen IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver |
| Gen V | Black / White / Black 2 / White 2 |
| Gen VI | X / Y / Omega Ruby / Alpha Sapphire |
| Gen VII | Sun / Moon / Ultra Sun / Ultra Moon |

For Gen I and II entries intended for the 3DS-era transfer chain, the provenance target is the Nintendo 3DS Virtual Console release. LegacyBox records the VC source explicitly instead of pretending a physical Game Boy cartridge directly entered Bank.

## Trainer policy

The default player identity is:

- OT: **Doug**
- Language: English
- Trainer IDs: generated per origin save/game according to that generation's actual ID rules

`ot_policy: trainer` means the encounter may use the player trainer identity and the effective OT is **Doug**.

`ot_policy: fixed_event` means a real distribution controls identity fields. LegacyBox must preserve those values exactly and must **not** replace them with Doug.

## Fixed event identity

A fixed-event entry carries an `event_identity` object. It can preserve:

- OT
- TID
- SID where applicable
- language locks
- distribution identifier
- Fateful Encounter state
- additional locked distribution fields

The generator treats those values as immutable. A fixed-event entry without an event identity is structurally invalid and cannot proceed to generation or legality validation.

## Provenance fields

Each curated entry records:

- source game
- source platform
- original generation
- region
- encounter method
- origin kind
- migration route
- player OT policy or fixed event identity

The resulting PK7 may therefore retain a chain such as:

```
Emerald / Game Boy Advance / Hoenn
  -> Pal Park
  -> Generation IV
  -> Poké Transfer
  -> Generation V
  -> Poké Transporter
  -> Pokémon Bank
```

while the immutable provenance record continues to identify Emerald as the original source.

## Important distinction

LegacyBox can generate data that is **legality-consistent with a documented encounter template**. That does not make a generated individual historically legitimate or prove it was actually obtained through normal gameplay.

## Event and Mythical exceptions

Some event-only Pokémon cannot legally use Doug as OT because their distribution fixes OT/TID/language or other values. Those entries remain part of the Heritage Dex with the required distribution identity preserved.

Where a species has a genuine player-OT encounter route, that route may be preferred when it meets the Heritage Dex provenance goal without inventing event metadata.

## Curation rule

The 807-entry index is only an inventory. An entry remains `pending_encounter_template` until a real encounter template is selected and passed through the legality backend. Source-generation ranges alone are never treated as proof of legality.


## Encounter-data verification

A curated encounter is not accepted from a single fan database. LegacyBox uses three layers:

1. **Bulbapedia** for encounter/gift tables, transfer behavior, and distribution notes.
2. **Serebii** for an independent location/level/moves/ability cross-check.
3. **PKHeX.Core** against the generated PKM as the machine legality gate.

A template may be marked `cross_checked` only when it contains both Bulbapedia and Serebii references. If the sources disagree, the manifest records the conflict and generation is blocked until the game data/PKHeX result resolves it.

The checked fields include, where applicable: source title, encounter location, encounter level/range, initial moves, level-up moves needed for the target level, ability, held item, OT policy, event locks, and transfer-specific transformations.

## Shiny policy

Shinies are curated rather than randomized across the whole dex. `force_shiny` is only a request; it must still pass the encounter's shiny restrictions and PKHeX legality check. Fixed distributions retain their exact shiny/non-shiny state.

For VC Gen I, a Pokémon may become Shiny when transported based on its Gen I DVs. That is recorded as a separate shiny phase rather than pretending the shiny state was visible in the original game.

## Pokérus policy

A small curated subset may be active or cured Pokérus. The acquisition generation is recorded separately from encounter provenance.

Poké Transporter strips Pokérus from Virtual Console Gen I/II Pokémon, so a VC-origin Pokémon that has Pokérus in its final Gen VII representation must have acquired it **after** transfer into Gen VII. LegacyBox rejects a VC trait plan that claims otherwise.
