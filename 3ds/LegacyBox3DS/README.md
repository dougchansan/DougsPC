# LegacyBox3DS

LegacyBox3DS is the local 3DS-side staging component for LegacyBox.

It **does not** implement, emulate, proxy, or reverse-engineer Pokémon Bank's private network protocol. Nintendo's official Pokémon Bank application remains the component that authenticates to the service and performs the cloud transfer.

## Current v0 flow

```
LegacyBox validated PK7 batch
        |
        v
SD: /3ds/LegacyBox/inbox/current/
        |
        v
LegacyBox3DS
  - parses LBX1 manifest
  - verifies PK7 file sizes
  - verifies SHA-256 for every file
  - marks batch ready
        |
        v
PKSM LegacyBox Batch Injector
  - checks target slots are empty
  - injects PK7s into USUM PC boxes
        |
        v
Exit PKSM normally / save changes
        |
        v
Official Pokémon Bank
        |
        v
Official Bank -> HOME Moving Key
```

The last two steps intentionally remain in Nintendo's official clients.

## SD layout

```
/3ds/LegacyBox/
  inbox/current/
    batch.tsv
    001_Bulbasaur.pk7
    ...
  ready.flag
```

## Building

Requires the current devkitPro 3DS toolchain:

```bash
cd 3ds/LegacyBox3DS
make -j
```

The resulting `LegacyBox3DS.3dsx` can be run through a Rosalina-based Homebrew Launcher. A future CIA build can support direct app chainloading, but it is not required for batch staging.
