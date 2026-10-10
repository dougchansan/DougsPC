# Third-party components

Doug's PC links PKSM-Core, copyright (C) 2016–2025 Bernardo Giordano,
Admiral Fish, piepie62, and its contributors. It is licensed under GPLv3 or
later, with additional terms requiring preservation of author notices and
accurate attribution of modified material. The complete license is in LICENSE.
The native save build and its corresponding source are distributed under those
terms. Doug's PC is a separate application; its Moon adapter and UI are our work,
not an official PKSM release.

Unmodified upstream source: https://github.com/FlagBrew/PKSM-Core
Pinned revision: d9bca6fcdf622760a0aa963565b102276e2ae2a3
Its memecrypto and pcg-cpp submodules retain their upstream licenses. The recursive
checkout selects memecrypto bd3b16ee2c12bd9ee353cf3098cb998e7880638c
and pcg-cpp 428802d1a5634f96bcd0705fab379ff0113bcf13. The build embeds
PKSM-Core's personal data files in RomFS. PKSM-Core handles PK7 parsing,
receiving-trainer updates, box encryption, Pokédex flags, block checksums and
Sun/Moon save signing.

The archive access sequence was checked against PKSM's TitleLoader and Archive
implementations. Our adapter uses libctru directly and supports Moon only.

PokéSprite artwork remains credited and licensed separately in the sprite pack.
PKHeX is used by the PC helpers and CI; it is not a runtime 3DS legality engine.

## PKHeX helpers

The optional PC helpers link against PKHeX.Core under GPL-3.0-or-later and must
be distributed and used under the applicable PKHeX/GPL terms. The Python core
continues to use validation as an external adapter boundary.
