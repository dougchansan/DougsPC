using System.Text.Json;
using PKHeX.Core;

if (args.Length is < 2 or > 3)
{
    Console.Error.WriteLine("usage: LegacyBox.EncounterCatalog <national-dex> <GameVersion enum> [form]");
    return 64;
}

if (!ushort.TryParse(args[0], out var species) || species == 0)
{
    Console.Error.WriteLine("invalid species");
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

var trainer = new SimpleTrainerInfo(version)
{
    OT = "Doug",
    Language = (int)LanguageID.English,
};

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

Console.WriteLine(JsonSerializer.Serialize(new
{
    requestedSpecies = species,
    requestedForm = form,
    requestedVersion = version.ToString(),
    count = encounters.Length,
    encounters,
}, new JsonSerializerOptions { WriteIndented = true }));

return encounters.Length == 0 ? 2 : 0;
