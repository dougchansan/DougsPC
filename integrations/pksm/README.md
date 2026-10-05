# PKSM integration

This is the first save-writing adapter for LegacyBox3DS.

Why PKSM first:

- PKSM already supports CFW access to Gen I–VII saves.
- Its script API exposes `sav_inject_pkx`, PC box access, validity checks, and save-specific transfer handling.
- This lets LegacyBox prove the full staging workflow before we duplicate all Gen VII save/checksum logic in the standalone app.

## Install

Copy `legacybox-batch.c` into PKSM's scripts directory using the same process as other custom PKSM C scripts.

The script expects:

```
/3ds/LegacyBox/inbox/current/batch.tsv
/3ds/LegacyBox/inbox/current/*.pk7
/3ds/LegacyBox/ready.flag
```

## Safety behavior

The script:

1. refuses to run if LegacyBox3DS has not marked the batch READY;
2. parses the entire target box/slot map;
3. checks every target box exists;
4. checks every target slot is empty;
5. reads each PK7 as Gen VII stored-format data;
6. asks PKSM to structurally validate it;
7. performs **zero writes until the whole batch preflight passes**;
8. injects the batch sequentially;
9. leaves normal PKSM save commit/exit behavior intact.

It intentionally does not interact with Pokémon Bank's network service.
