# LegacyBox3DS frontend — first implementation

Base: dougchansan/LegacyBox main `ed584c2241e659ab42ae7de9d0f941bf72a1b2de`.
No remote push, PR, merge, or release was performed.

## Implemented

- Native dual-console interface with touchscreen tabs, species selection, and button controls.
- Full #001–#807 species inventory generated from the existing heritage index. Generation filtering and eight-entry paging.
- Batch level, nature, shiny request, and destination-box options.
- Heritage #001–#009, living-dex request, shiny-heritage request, and clear presets.
- Atomic LBXR2 request (LBXR1 remains readable) export to `/3ds/LegacyBox/request.tsv`.
- PC request resolver preserving reviewed encounter metadata and rejecting missing templates, duplicate species, unknown/locked shiny eligibility, fixed-event edits, and invalid capacity.
- Offline exact-template assembly: choose Pokémon from an installed passed batch pack; copy their original PK7 bytes into a compatible LBX1 bundle. Hashes are verified before committing the replacement. This is assembly of already generated candidates, not a native encounter generator or legality engine.
- Previous batch preserved in `inbox/frontend-backup`; old READY flag invalidated. Leftover staging/backup folders block another assembly until reviewed.
- Existing SHA256 verification and PKSM handoff; X re-verifies immediately before READY. READY writes are now checked and atomic.
- Persistent manual checklist tied to the concrete batch ID: staged → save/readback → Bank box → HOME receipt. These are operator confirmations, not automated evidence.
- Rosalina companion using Luma's unsigned, active-low input packet format. Dry run by default; checkpointed segments; resumable journal keyed to profile SHA256; button release on interruption.

## Controls

L/R changes tabs. START exits. Touch the top row of the bottom screen to select a tab.

Browse: Up/Down selects a row; Left/Right pages eight entries; A or a row tap toggles selection; B clears; Y cycles generation filter; SELECT or the bottom action row assembles exact installed templates; X or the final action row saves a PC generation request.

Options: Up/Down selects a field; Left/Right changes it; tapping a field increments it. Level 0, Nature Template, and Shiny Template preserve original template settings. Destination box is displayed one-based, encoded zero-based.

Presets: Up/Down chooses, A applies; tapping a preset also applies it. Presets replace the selection. The shiny preset changes the shiny request; other presets retain options, so review Options before exporting/assembling.

Batch: A verifies; X verifies again and writes READY; Y reloads SD state. A failed verification invalidates the existing READY flag.

Queue: A confirms the next observed step; B resets this batch's confirmations. Staging must occur in the Batch tab before later confirmations.

## Scope still pending

- A complete on-device encounter generator and legality engine.
- Per-Pokémon customization, search keyboard, form selection, IV/EV/move editors, and image artwork.
- Reviewed templates for the rest of #001–#807, complete event collections, and a per-origin-game sampler.
- Shiny-heritage generation remains blocked until the chosen templates explicitly record reviewed `shiny_allowed: true`. Unknown eligibility is not treated as allowed.
- Automated screen recognition and a verified Bank box-transfer input profile. Only a harmless D-pad probe is supplied. Do not infer Bank navigation from timing or from successful UDP sends.
- Per-box transfer evidence for multi-box runs. This first checklist is batch-wide; validate the nine-entry milestone before scaling.
- C# seeded sampler compilation and full PKHeX legality/replay validation, native build, actual touchscreen layout, real-save readback, and official Bank → HOME transfer have not been tested here.

## Prepare and build locally

Use the included patch against a clean checkout of the exact base, or adapt it after reviewing newer main changes. Do not overwrite unrelated work.

```bash
git switch -c codex/3ds-frontend
git apply --check /path/to/LegacyBox3DS-frontend.patch
git apply /path/to/LegacyBox3DS-frontend.patch
python scripts/build_frontend_catalog.py
PYTHONPATH=src ASAN_OPTIONS=detect_leaks=0 python -m unittest discover -s tests -v
make -C 3ds/LegacyBox3DS -j2
```

Host-side native tests need `gcc`. On Windows, run them in WSL or a host GCC environment. The frontend build uses the existing devkitPro Makefile and automatically includes the new C sources.

Install the resulting `LegacyBox3DS.3dsx` and `.smdh` into the SD homebrew app folder. The implementation package does not contain compiled 3DS binaries, game saves, ROMs, keys, or PK7 candidates.

## Offline template milestone

Use your existing generator output directory containing concrete PK7 files and its matching full-pass PKHeX `batch-report.json`:

```bash
python scripts/build_template_pack.py /path/to/passed-batch /path/to/new-template-pack
```

Copy the new pack directory to `/3ds/LegacyBox/templates/`. Never substitute the repository's static curation inventory for a fresh candidate-byte legality report. The packer requires each file's reported SHA256 to match its bytes.

In LegacyBox3DS, select Heritage #001–#009, preserve all template options, and choose an empty target box. SELECT assembles installed templates. Verify in Batch; X marks READY. Exit, run the existing v0.3 PKSM injector, save normally, and verify the actual Pokémon and original occupied slots before marking save/readback confirmed. Continue through official Bank and HOME; confirm only observed outcomes.

Review/preserve `inbox/frontend-backup` after success before removing or moving it. Failed assemblies may leave `inbox/frontend-new` for inspection. The app deliberately refuses to overwrite either folder on the next attempt.

## PC-assisted customization

Copy `request.tsv` off SD and resolve it:

```bash
python scripts/frontend_request.py /path/to/request.tsv /path/to/request-manifest.json
```

Default template manifest is the reviewed #001–#009 set. Requests outside it fail with the missing species list. Resolution writes a generation manifest marked UNVERIFIED; it does not generate Pokémon or grant a legality verdict.

Use the existing PKHeX batch generator with that manifest, then `scripts/build_3ds_bundle.py` against its fresh passed output. Supply `--start-box` using `frontend_request.start_box` from the resolved JSON. Copy the resulting bundle to SD only after preserving the current one and removing its old READY flag. A fresh verification/staging step is required.

## Rosalina hardware probe

Enable Rosalina InputRedirection on the 3DS and use the observed LAN address. Prior working address was `192.168.50.154`; confirm it is still the device's address.

```bash
python scripts/rosalina_companion.py integrations/rosalina/dpad-probe.json --host 192.168.50.154
python scripts/rosalina_companion.py integrations/rosalina/dpad-probe.json --host 192.168.50.154 --execute
```

The first command sends nothing. The second requires READY before sending and OBSERVED after the result. Restarting an incomplete segment requires returning to its stated checkpoint first; do not retry uncertain inputs on a different screen. Journals record operator-observed input segments only, not Bank/HOME acceptance.

Map one Bank box movement on the actual device, add short segments separated by loading/selection/save/transfer checkpoints, and demonstrate a single verified transfer before broadening. The official HOME Moving Key remains manual.

## Sandbox validation

35 Python tests passed, including host-compiled native request export/PC resolution, exact-template assembly, deterministic bundle IDs, preservation of an old batch, stale READY invalidation, tampered-template rejection before replacement, and unsigned Rosalina packet/release behavior. Native request core ran with AddressSanitizer/UBSan (leak detection disabled because this runtime restricts process inspection).

The UI C source also passed `-Wall -Wextra -Werror` syntax checking against a minimal libctru API stub. That is a source check, not a libctru build, render, or hardware result. devkitARM/libctru are unavailable in this environment.

## RNG settings added

Options now has Default random / Fixed transfer seed, an eight-digit unsigned hex seed, and an IV-search budget of 5,000 / 50,000 / 250,000 rolls per species. In the Seed field, X advances the digit; Left/Right edits its hex value. A row tap increments the active digit. Both 00000000 and FFFFFFFF are preserved.

LBXR2 includes RNG mode, seed, and budget. The request resolver carries these into the generation manifest. Offline assembly rejects non-default RNG settings because it preserves original files.

The Gen I VC helper source now reads the search budget and includes a deterministic transfer-field sampler for fixed mode. Seeds are derived per species/template using SHA256; an independent SplitMix64 sampler chooses EC, PID, and IV rolls with the VC flawless-IV minimum. Native encounter resolution remains in PKHeX. Source shiny state and all existing legality checks remain in place. Seed, sampler version, attempts used, actual IVs, PID, and EC are recorded in the report.

This controls synthetic candidate transfer rolls. It does not reproduce an original Game Boy/GBA/DS encounter RNG, manipulate the running game, synchronize real hardware frames, or guarantee identical full files. Source encounter randomness, dates, runtime and PKHeX revision can affect other fields. Full output replay would need a versioned source fixture and fixed date policy.

The seed transport and rejection tests passed here. The .NET 10 SDK and PKHeX build dependency are absent, so the new C# sampler has NOT been compiled or validated by PKHeX here. Before using its output, the local agent must compile it, generate the nine-entry batch in default and seeded modes, require full legality passes, compare actual transfer fields across two runs with the same seed and identical source/shiny policy, and confirm a different seed changes transfer rolls. Do not promote generated bytes without those checks.
