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
