# PKHeX legality validator helper

This optional helper validates a concrete PKM file with PKHeX.Core.

It is intentionally an out-of-process boundary:

```
LegacyBox core -> generated .pk3/.pk4/.pk5/.pk6/.pk7 -> helper -> JSON legality result
```

The helper does not generate Pokémon and does not fabricate Bank/HOME metadata.

## Build

The project expects a PKHeX checkout at:

```
external/PKHeX
```

Then:

```bash
dotnet build helpers/pkhex-validator/LegacyBox.PKHeXValidator.csproj
dotnet run --project helpers/pkhex-validator/LegacyBox.PKHeXValidator.csproj -- candidate.pk7
```

## Licensing

PKHeX and PKHeX.Core are GPL-3.0-or-later. Any distribution of this helper linked against PKHeX.Core must comply with the applicable GPL terms. The main LegacyBox core communicates with this optional helper through files/stdout rather than linking PKHeX.Core directly.
