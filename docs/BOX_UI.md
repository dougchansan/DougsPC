# Graphical box frontend

Browse now displays a 6-column, 5-row species catalog with 30 icons per page.
The active tab is teal with a gold underline. Gold outlines identify the
cursor, and teal markers identify selected species. The top screen shows
the focused species, enlarged icon, destination and operation result.

This is the species catalog, not a live save or Bank box viewer. Selecting a
shiny request previews a shiny icon; this never grants shiny eligibility.
Offline template assembly, PKSM empty-slot checks, integrity checks and
manual transfer observations retain their existing semantics.

## Install

Copy LegacyBox3DS.3dsx and LegacyBox3DS.smdh to SD:/3ds/LegacyBox3DS/.
Copy the contents of the LegacyBox-sprite-assets artifact to
SD:/3ds/LegacyBox/assets/. Preserve existing app files before updating.
Missing, truncated or wrong-format packs display question-mark placeholders
and do not prevent request export or batch verification. Restart after
installing assets. No sprites are downloaded by the 3DS.

## Controls

- L/R: change tab; touch a tab to select it.
- Browse D-pad: move across the grid; Up/Down move six cells.
- Touch the arrows beside the catalog label: previous/next page.
- A or a cell tap: toggle that species. B clears; Y changes generation.
- SELECT or Assemble button: assemble installed validated templates.
- X or Save Request button: export the existing PC generation request.
- Options, presets, batch actions and queue retain their button controls.

Base species #001-807 regular/shiny icons come from msikma/pokesprite at
c5aaa610ff2acdf7fd8e2dccd181bca8be9fcb3e. Forms remain outside this UI's
scope. The build copies upstream attribution/license into the asset pack.

Host tests exercise actual render code, framebuffer bounds, final-page
hitboxes and directional navigation. CI must build the native 3DS binary
and all 1614 icons before installation. Hardware layout/performance and
Bank/HOME outcomes require a device test.

## Device RNG and level preservation

Level **Preserve** is the existing internal value 0. Left from 1 returns to
Preserve; right from Preserve chooses 1. It retains the selected template's
level, which need not be the same for every species.

RNG cycles PC default / PC fixed seed / Device rolls. Device rolls runs
LBX-DEVICE-ROLLS-1 (SplitMix64) in native C on the 3DS. It exports one EC,
raw PID and packed set of six IVs per species in LBXR3 request.tsv; the PC
uses those values without rerolling or searching IVs. The device guarantees
three perfect VC IVs, or five for Mew. This mode supports reviewed Gen I VC
Red/Blue/Yellow templates only, species 1-151; fixed events stay locked.
The PC constructs the Pokemon file, normalizes PID to the source shiny
state and runs actual PKHeX validation. These are generated transfer rolls,
not historical encounter RNG or proof of an authentic encounter.

The app chooses a starting seed from the device clock/tick. The Seed row
allows manual editing with Left/Right and X to select a digit; Y chooses
a new clock/tick seed. The same seed/species reproduces the exported rolls.
It does not guarantee the entire host-generated file is byte-identical.
IV Search shows One local candidate in device mode: desired IV targets may
not be met. There is no local IV search in this version.

Choose Device rolls, select supported species and press X in Browse to
export request.tsv. Resolve it with scripts/frontend_request.py and use
the PC batch generator, then build a passed bundle using the normal tools.
SELECT still assembles exact SD templates and refuses RNG/customization;
requests cannot be marked READY or injected before the PC preparation.

Doug's PC is the display name. Existing LegacyBox SD paths and binary
filenames remain compatible with the installed injector and sprite packs.
