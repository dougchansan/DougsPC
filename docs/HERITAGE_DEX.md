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


## Competitive profile policy

Competitive optimization is a **derived layer** and never overrides encounter provenance.

Source hierarchy:

1. Smogon/Showdown analysis for a documented competitive role, nature, EV spread, item, ability and moveset.
2. Bulbapedia/Serebii and the PKHeX encounter/move database for whether those fields are obtainable from the selected provenance.
3. PKHeX legality analysis on the concrete generated PKM.

Rules:

- EVs may be trained to the selected competitive spread when the format permits it.
- Random nature/IV encounters may be generated toward competitive values only when the encounter mechanics allow those values.
- Fixed event nature, IVs, ability, shiny state, OT and other locked fields remain fixed.
- A competitive recommendation that requires an impossible hidden ability is adapted or rejected. Provenance wins.
- Pure special attackers prefer 0 Attack IV where legal to reduce Foul Play/confusion damage.
- Physical/mixed attackers normally prefer 31 relevant IVs.
- Hidden Power sets use the exact required IV pattern rather than six arbitrary 31s.
- Gen I/II VC nature is realized through pre-transfer Experience modulo 25; VC IVs must respect Transporter's randomized-IV rules and guaranteed perfect IV count.
- Hyper Training is not treated as rewriting the underlying IV value.

Every competitive profile records whether it is a direct Smogon set or a provenance-adapted version.


## Archive Mode variants

The living dex answers “one representative per species.” Archive Mode answers “which legally distinct versions of this species can exist?”

LegacyBox treats a variant as materially distinct when at least one of these changes:

- origin game or generation
- persistent form or regional form
- ability slot / Hidden Ability availability
- fixed event distribution identity
- shiny eligibility or fixed shiny state
- origin language/region when it affects stored provenance
- special ribbon or move provenance
- GameCube Shadow -> Purified lineage
- transfer-specific provenance that survives into later generations

We do **not** blindly take the Cartesian product of every field. Each variant must correspond to a real obtainable path.

### Colosseum and XD

Pokémon Colosseum and Pokémon XD: Gale of Darkness are first-class Generation III origin sources.

A Shadow-origin archive record preserves:

- Colosseum vs XD
- snag encounter/location
- original Shadow encounter level
- XD Shadow Boost where applicable
- generated Nature/IV/gender/PID constraints
- purification status
- National Ribbon
- XD purification-exclusive special move, when applicable
- later GBA -> DS -> Bank migration chain

A Shadow Pokémon itself cannot be exported to the GBA core games. The transferable lineage is the **purified** Pokémon. Purification grants the National Ribbon, and the purified Pokémon can then be traded to Ruby/Sapphire/Emerald/FireRed/LeafGreen before continuing through later generations.

Nature and IVs are not globally fixed for Shadow Pokémon: they are generated when first encountered and then remain fixed for that save/Shadow Monitor entry. Archive generation must therefore create a valid Gen III RNG-consistent specimen rather than assigning arbitrary modern IVs after the fact.

Competitive optimization happens only after those origin constraints are satisfied. EVs may be trained after purification; Shadow battles themselves do not accumulate EVs.
