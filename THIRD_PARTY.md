# Third-party licensing

## PKHeX legality helper

`helpers/pkhex-validator` is an optional executable designed to link against PKHeX.Core.

PKHeX.Core is provided by the PKHeX project and is licensed GPL-3.0-or-later. The helper must be distributed and used in compliance with the applicable PKHeX/GPL terms.

The Python LegacyBox core does not contain or link PKHeX.Core; it treats concrete PKM validation as an external adapter boundary.
