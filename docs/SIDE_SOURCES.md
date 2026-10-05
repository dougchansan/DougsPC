# LegacyBox side-source catalog

LegacyBox Archive Mode treats official side games, demos, Bank distributions, QR/game-based gifts, and transfer peripherals as first-class provenance sources.

The machine-readable source of truth is `data/side-sources.json`.

## Generation III

### Pokémon Box Ruby & Sapphire

Four special Eggs are provenance-significant because their moves are otherwise unavailable from normal Gen III breeding:

- Swablu — False Swipe
- Zigzagoon — ExtremeSpeed
- Skitty — Pay Day
- Pichu — Surf

The hatching save supplies OT/TID. These Eggs may hatch Shiny. Surf Pichu must forget Surf before Pal Park.

### Colosseum Bonus Disc / Pokémon Channel

Distinct fixed distributions include:

- WISHMKR Jirachi — US Bonus Disc
- Ageto Celebi — Japanese Bonus Disc
- CHANNEL Jirachi — European/Australian Pokémon Channel
- Japanese Bonus Disc Pikachu
- Japanese Card e Room Shadow Togepi, Mareep, and Scizor

Jirachi variants are kept separate because their RNG method, OT/TID, held-item possibilities, region and distribution source differ.

### Colosseum / XD

Native gifts/starters and Shadow->Purified lineages are separate archive classes. Purified Shadow Pokémon retain the National Ribbon and, for XD, may carry purification-exclusive legacy moves.

## Generation IV

### Pokémon Ranger

Ranger Manaphy is an Egg lineage, not an ordinary fixed Pokémon. The original recipient cannot hatch it Shiny, but trading the Egg before hatching permits a legitimate Shiny result under the Gen IV TID/SID rules.

### My Pokémon Ranch

Hayley's trades are fixed side-game provenance. Mew and Phione are particularly important, but the archive should eventually enumerate every unique Hayley trade with its language-specific OT and fixed/random field rules.

### Pokémon Battle Revolution

The PKTOPIA distributions include Surf Pikachu plus Electivire and Magmortar. LegacyBox records historical moves separately from later-transfer moves because Surf must be forgotten before Pikachu can move through Poké Transfer.

### Pokéwalker

Pokéwalker provenance includes course, encounter slot, special move set, and Pokéwalker RNG. Pokéwalker PID generation cannot produce Shiny Pokémon.

## Generation V

### Pokémon Dream World / Pokémon Global Link

Entree Forest encounters are a major source of Hidden Abilities. Ball choice is provenance-sensitive: a Dream Ball is provided, but other legally available balls can be used. Gender and special-move restrictions vary by encounter.

### Pokémon Dream Radar

Dream Radar encounters have:

- Hidden Ability when the species has one, except Rotom
- Dream Ball
- Pokémon Dream Radar met location
- B2W2 player ownership
- level determined by badge count, from 5 through 40

These are distinct from Dream World/Entree Forest origins.

## Generation VI

### ORAS Special Demo

The transferable Mega Pokémon are region-specific:

- Glalie for American/PAL/Korean/Taiwanese-region demos
- Steelix for Japanese-region demos

They transfer through Pokémon Link and retain demo-specific provenance.

### Pokémon Bank distributions

Bank gifts are their own source family. Examples include the original Bank Celebi and Hidden Ability Johto first partners. Individual distributions must preserve their exact date window, moves, ribbon, ability, language and receiving-game rules.

## Generation VII

### Sun/Moon Special Demo

Battle Bond Greninja is a unique lineage. Its transfer behavior, fixed OT/TID/IV/EV data, Souvenir Ribbon and Battle Bond Ability are not interchangeable with normal Greninja.

### Pokémon Bank distributions

Bank continued to distribute distinct Gen VII gifts, including Hidden Ability Alola first partners. Each Wonder Card/distribution is archived separately when its stored data is materially distinct.

### QR game-based gifts

QR gifts such as Magearna and Partner Cap Pikachu are different from generic Pokédex QR scans and Island Scan encounters.

### Island Scan

Island Scan is an encounter source rather than a fixed gift. Species, day, island, version, level and special move are all provenance fields.

## Verification policy

Each archive variant follows:

```
source registry
  -> PKHeX encounter/event candidate
  -> Bulbapedia + Serebii cross-check
  -> native-format generation
  -> migration transformations
  -> PKHeX legality pass
  -> approved archive variant
```

A source family being catalogued does **not** mean every individual Pokémon from that source has already been generated. The registry separates coverage of the acquisition ecosystem from the slower per-variant legality work.
