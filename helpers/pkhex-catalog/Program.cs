using System.Text.Json;
using PKHeX.Core;

if (args.Length is < 2 or > 3)
{
    Console.Error.WriteLine("usage: LegacyBox.EncounterCatalog <national-dex|start-end> <GameVersion enum> [form]");
    return 64;
}

if (!Enum.TryParse<GameVersion>(args[1], ignoreCase: true, out var version))
{
    Console.Error.WriteLine("invalid GameVersion");
    return 64;
}

byte form = 0;
if (args.Length == 3 && !byte.TryParse(args[2], out form))
{
    Console.Error.WriteLine("invalid form");
    return 64;
}

static bool TryParseSpeciesRange(string value, out ushort start, out ushort end)
{
    start = end = 0;
    var parts = value.Split('-', 2, StringSplitOptions.TrimEntries);

    if (parts.Length == 1)
    {
        if (!ushort.TryParse(parts[0], out start) || start == 0)
            return false;
        end = start;
        return true;
    }

    if (!ushort.TryParse(parts[0], out start) || !ushort.TryParse(parts[1], out end))
        return false;
    return start > 0 && end >= start;
}

if (!TryParseSpeciesRange(args[0], out var startSpecies, out var endSpecies))
{
    Console.Error.WriteLine("invalid species or species range");
    return 64;
}

var trainer = new SimpleTrainerInfo(version)
{
    OT = "Doug",
    Language = (int)LanguageID.English,
};

object BuildResult(ushort species)
{
    var template = EntityBlank.GetBlank(trainer);
    template.Species = species;
    template.Form = form;
    template.Gender = template.GetSaneGender();

    var encounters = EncounterMovesetGenerator
        .GenerateEncounters(template, trainer, ReadOnlyMemory<ushort>.Empty, version)
        .Select(enc => new
        {
            type = enc.GetType().Name,
            name = enc.Name,
            longName = enc.LongName,
            species = enc.Species,
            form = enc.Form,
            version = enc.Version.ToString(),
            generation = enc.Generation,
            levelMin = enc.LevelMin,
            levelMax = enc.LevelMax,
            location = enc.Location,
            isEgg = enc.IsEgg,
            ability = enc.Ability.ToString(),
            fixedBall = enc.FixedBall.ToString(),
            shiny = enc.Shiny.ToString(),
        })
        .Distinct()
        .ToArray();

    return new
    {
        requestedSpecies = species,
        requestedForm = form,
        requestedVersion = version.ToString(),
        count = encounters.Length,
        encounters,
    };
}

if (startSpecies == endSpecies)
{
    var result = BuildResult(startSpecies);
    Console.WriteLine(JsonSerializer.Serialize(
        result,
        new JsonSerializerOptions { WriteIndented = true }
    ));
    return 0;
}

var results = Enumerable
    .Range(startSpecies, endSpecies - startSpecies + 1)
    .Select(i => BuildResult((ushort)i))
    .ToArray();

Console.WriteLine(JsonSerializer.Serialize(new
{
    requestedRange = new[] { startSpecies, endSpecies },
    requestedForm = form,
    requestedVersion = version.ToString(),
    results,
}, new JsonSerializerOptions { WriteIndented = true }));

return 0;
