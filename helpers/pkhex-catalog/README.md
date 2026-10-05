# PKHeX encounter catalog helper

This helper asks PKHeX.Core for encounter templates that can legally produce a requested species in a requested source game.

Example:

```bash
dotnet run --project helpers/pkhex-catalog/LegacyBox.EncounterCatalog.csproj -- 3 RD
```

That query asks for legal Red VC encounter origins capable of eventually producing Venusaur. Evolution-chain resolution is handled by PKHeX.Core, so evolved species can resolve back to a starter, wild encounter, gift, egg, or event origin.

The JSON output includes the PKHeX encounter type, version, generation, level range, location ID, ability permission, Ball, shiny policy, and descriptive encounter name.

This is a **candidate catalog**, not an automatic provenance selector. LegacyBox still cross-checks the selected candidate with Bulbapedia/Serebii and validates the final generated PKM separately.

PKHeX.Core is GPL-3.0-or-later; see `THIRD_PARTY.md`.
