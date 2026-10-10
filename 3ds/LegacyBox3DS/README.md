# Doug's PC on 3DS

The Moon workflow is now **Browse → prepare → preview → backup and save → Bank**.
Request export remains available with X. Pokémon Bank and HOME use their official
applications; this app does not connect to their servers.

## Install and first test

1. Replace `SD:/3ds/LegacyBox3DS/LegacyBox3DS.3dsx` and `.smdh` with the new build.
2. Extract the `DougsPC-reviewed-starter-templates` artifact at the SD root.
   It installs `SD:/3ds/LegacyBox/templates/templates.tsv` and the reviewed PK7 files.
   Existing sprite packs stay at `SD:/3ds/LegacyBox/assets/`.
3. Close Moon. Insert its cartridge, or use the installed digital Moon title.
   The cartridge takes priority if both are present. The preview names the selected medium.
4. In Browse choose just Bulbasaur. Options: RNG **Device search**, IV search **5000**,
   level/nature/shiny **Preserve/Template**, start box an empty box (e.g. Box 2).
5. Press SELECT. Search results are exported to `request.tsv`; the app builds the
   candidate from the reviewed template, reads Moon, and prepares edits in memory.
6. Review the target and count. B cancels without changing Moon. A writes a verified
   backup, saves the edited image, commits the archive, and compares the entire
   readback against the prepared image. Wait for the explicit success message.
7. Exit Doug's PC. Inspect the box in Moon before the first Bank deposit. After this
   hardware test succeeds, the regular procedure can go straight to Pokémon Bank.

This is a new direct-save path. Host/CI checks do not replace the first console test.
An archive access failure is reported without changing the game. Some launcher/CFW
configurations may need different save access permissions; do not retry through a
blind controller sequence.

## Supported native generation

- Device search plus reviewed Gen I VC starter templates #001–#009.
- Device-generated EC/PID/IVs; PID normalized to preserve or request shiny status.
- Nature options and levels at or above each reviewed template's current level.
  Lower levels are rejected because the reviewed evolution/move recipe may no
  longer apply. Moves, EVs, origin and OT stay from the reviewed template.
- Preserve mode copies installed reviewed templates without randomization.
- Device search profiles are compiled into the app; missing recipes fail closed.
- The app performs structural/integrity checks. Full PKHeX legality runs in CI on
  native outputs; a complete runtime legality engine is not built into the 3DS app.
  These checks do not establish authentic encounter history or HOME acceptance.

Other species require reviewed templates and supported native recipes. The 807
catalogue entries are not a claim of generation coverage. PC default/fixed RNG
with edits still uses X export and the PC helper.

## Existing batch import and backup behavior

Batch tab: A verifies, X prepares the loaded SD batch for Moon, Y reloads.
SELECT retains the PKSM READY fallback. Preparing a batch does not write the game.
Imported PK7 files must pass size/checksum/hash checks; their legality must have
been established before packaging.

Every destination must be empty; Box 8 is protected. The original save must still
match the preview before writing. A content-addressed original backup is stored as
`/3ds/LegacyBox/backups/moon-<SHA256>.main`, then independently read back. Existing
backup files must match exactly. Never remove your original backups to make room.
On a failed write, the app attempts to restore and verify the original. A failure
or unverified restoration is never reported as successful; use the backup with a
save manager before continuing to Bank. As with other save managers, power must
remain on while writing. No software transaction can guarantee recovery from
power loss during a cartridge commit.

Queue remains an operator-confirmed transfer checklist, not a Bank/HOME receipt.
The HOME Moving Key is entered in the official transfer workflow.

## Build

Requires current devkitPro devkitARM/libctru and an unmodified recursive checkout:

```sh
git clone https://github.com/FlagBrew/PKSM-Core external/PKSM-Core
git -C external/PKSM-Core checkout d9bca6fcdf622760a0aa963565b102276e2ae2a3
git -C external/PKSM-Core submodule update --init --recursive
make -C 3ds/LegacyBox3DS -j
```

The Makefile embeds upstream personal data in RomFS. See `THIRD_PARTY.md` and
`LICENSE` for PKSM-Core attribution and source/license requirements.
