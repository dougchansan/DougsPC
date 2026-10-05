# Heritage Dex provenance policy

LegacyBox Heritage Dex mode is intended to preserve visible origin history rather than generating every entry as if it came from the newest supported game.

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

For Gen I and II entries that are intended to pass through the official 3DS-era path, the provenance target is the 3DS Virtual Console releases because Poké Transporter officially supports those releases. Physical Game Boy cartridges do not directly connect to Bank.

## Trainer policy

The default player identity is:

- OT: **Kento**
- Language: English
- Trainer IDs: generated per origin save/game according to that generation's rules

`ot_policy: trainer` means the encounter may use the player trainer identity.

`ot_policy: fixed_event` means the real distribution fixes the OT (and potentially additional fields). In that case LegacyBox must preserve the distribution OT instead of forcing Kento.

## Important distinction

LegacyBox can generate data that is **legality-consistent with a documented encounter template**. That does not make the generated individual historically legitimate or prove it was actually obtained through normal gameplay.

## Event and Mythical exceptions

Some event-only Pokémon cannot legally use Kento as OT because their distribution data fixes OT/TID/language or other values. Those entries remain part of the Heritage Dex, but the manifest records the required fixed event identity.

Where a Mythical has a genuine player-OT encounter route, that route should be preferred if it produces the desired heritage without inventing event metadata.

## Generation transitions

The long-term migration model records both:

1. the original encounter provenance; and
2. the transformations applied while moving through later generations.

The original provenance is never overwritten merely because the Pokémon is currently stored as PK7 or later.
