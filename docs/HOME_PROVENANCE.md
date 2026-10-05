# Bank / HOME provenance policy

LegacyBox deliberately separates **local legality** from **historical legitimacy** and from **Nintendo server provenance**.

## Official Bank -> HOME facts

As of October 4, 2026:

- Pokémon Bank can still move Pokémon into Pokémon HOME.
- Pokémon HOME Premium is required for Bank transfers.
- The move is one-way: a Pokémon moved from Bank to HOME cannot go back to Bank.
- Nintendo has announced that Pokémon Bank service ends on **February 25, 2027 at 7:00 p.m. PST**, and Bank -> HOME transfers end with it.

LegacyBox therefore treats the current Bank window as an official migration opportunity, not as metadata we can reconstruct later.

## Four separate trust axes

### 1. Origin evidence

This answers: *where did LegacyBox get the pre-Bank Pokémon?*

- `generated_candidate` — created by LegacyBox from a reviewed legal encounter template.
- `imported_source_bytes` — imported from a real save/PKM file supplied by the user.
- `user_attested_original` — user says the specimen is historically original, but LegacyBox cannot independently prove that.
- `unknown`.

A generated Pokémon stays `generated_candidate` forever. Passing through real Bank/HOME does **not** retroactively make the original encounter historically legitimate.

### 2. Local PKM legality

This is the PKHeX/LegacyBox result:

- `UNVERIFIED`
- `PASSED`
- `FAILED`

A `PASSED` result means the file is consistent with known game mechanics and encounter constraints. It is not proof of how the Pokémon was actually obtained.

### 3. Official Bank transfer evidence

LegacyBox may record:

- `not_attempted`
- `user_confirmed_official_move`
- `not_applicable`

For now this is user/workflow evidence, not cryptographic proof. The 3DS Bank service does not expose a public receipt API that LegacyBox can authenticate.

### 4. HOME provenance evidence

For PK8+ snapshots LegacyBox may record:

- `pre_home`
- `tracker_observed`
- `user_confirmed_official_move_and_tracker_observed`
- `not_applicable`

A tracker is **observed**, never declared valid by LegacyBox.

## HOME tracker rule

PKHeX's current transfer verifier explicitly says it cannot validate the actual HOME tracker values because PKHeX is not the HOME server. It can only determine when a tracker should exist and flag a missing/zero tracker.

LegacyBox therefore follows a strict rule:

> Never fabricate, copy, transplant, or synthesize a HOME tracker.

If a Pokémon goes through HOME legitimately, HOME assigns/manages the tracker. Afterward LegacyBox may import a PK8/PK9 snapshot, record that a nonzero tracker was present, and store a SHA-256 hash of that snapshot as evidence.

The tracker value itself is not treated as a permanent provenance identifier.

## Why HOME acceptance is not enough

Community reverse-engineering shows that some upload paths have historically accepted malformed/missing tracker situations. That means “HOME accepted it” is weaker than “HOME server history proves every claimed origin detail.”

LegacyBox records these separately:

```
Origin: generated_candidate
PKHeX legality: PASSED
Bank transfer: user_confirmed_official_move
HOME provenance: tracker_observed
HOME server validity: unverifiable_locally
```

That is an honest record.

For an actually caught/imported Pokémon:

```
Origin: imported_source_bytes
PKHeX legality: PASSED
Bank transfer: user_confirmed_official_move
HOME provenance: tracker_observed
HOME server validity: unverifiable_locally
```

LegacyBox still avoids claiming server-side authenticity that it cannot independently inspect.

## Recommended evidence capture

Before Bank:

1. Save exact original/native PKM bytes.
2. Hash them.
3. Record source save/game and encounter template.
4. Run local legality validation.

After official Bank -> HOME:

1. Move the Pokémon into a compatible Switch game through HOME.
2. Export/read the resulting PK8/PK9.
3. Confirm a tracker is present when expected.
4. Hash the post-HOME snapshot.
5. Record the migration event and timestamp.
6. Never edit immutable historical fields after HOME if preserving provenance is the goal.

This gives LegacyBox a defensible audit chain without pretending to have access to Nintendo's private server database.


## HOME child data

Reverse-engineered HOME documentation indicates that HOME does not store only one flattened Switch-era Pokémon record. It maintains common/core data plus game-specific child data. A Bank -> HOME or GO -> HOME entry initializes the SWSH child representation first; additional child representations are created as the Pokémon visits compatible games.

This is why a copied tracker by itself is not equivalent to a genuine historical HOME path. The server-side record associated with that tracker can contain data not represented by simply editing one local PK8/PK9 file.

LegacyBox therefore preserves:

- the original pre-HOME snapshot;
- the post-HOME snapshot;
- hashes for both;
- observed tracker presence;
- which games were intentionally visited after HOME entry, when known.

It does **not** attempt to synthesize missing HOME child history.

## Immutable-after-HOME rule

For preservation-oriented records, LegacyBox treats these as immutable after the first HOME entry unless the official games/HOME themselves change them:

- PID / Encryption Constant;
- IVs;
- origin game;
- OT name/gender, TID and SID;
- language;
- met level/location/date;
- Egg met data;
- original Ball;
- gender;
- nature where not legitimately changeable in underlying data;
- ribbons that should not disappear;
- other format-specific values correlated to the original encounter.

Competitive EVs, moves, items, friendship and similar legitimately mutable gameplay data remain separate from immutable provenance.


## Bank/Transporter acceptance is not a legitimacy certificate

Poké Transporter and Pokémon Bank perform their own server-side checks, but passing those checks does not prove that a Pokémon was originally obtained through normal gameplay.

LegacyBox therefore interprets an official Bank/Transporter success only as:

- the official service accepted the data at that time; and
- the official migration path was used from that point forward.

It does **not** upgrade `origin_evidence: generated_candidate` to an original/caught status.

This keeps the audit record honest even if official checks are incomplete or change over time.
