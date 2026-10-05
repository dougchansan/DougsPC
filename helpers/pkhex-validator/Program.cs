using System.Text.Json;
using PKHeX.Core;

if (args.Length != 1)
{
    Console.Error.WriteLine("usage: LegacyBox.PKHeXValidator <pkm-file>");
    return 64;
}

var path = args[0];

try
{
    var data = File.ReadAllBytes(path);
    var pk = EntityFormat.GetFromBytes(data);

    if (pk is null)
    {
        Console.WriteLine(JsonSerializer.Serialize(new
        {
            parsed = false,
            valid = false,
            error = "PKHeX.Core could not parse the input as a PKM entity."
        }));
        return 2;
    }

    var legality = new LegalityAnalysis(pk);
    var hasTrackerField = pk is IHomeTrack;
    var trackerPresent = pk is IHomeTrack { HasTracker: true };
    var trackerRequired = HomeTrackerUtil.IsRequired(legality.EncounterMatch, pk);

    Console.WriteLine(JsonSerializer.Serialize(new
    {
        parsed = legality.Parsed,
        valid = legality.Valid,
        species = pk.Species,
        format = pk.Format,
        generation = pk.Generation,
        version = pk.Version.ToString(),
        encounterType = legality.EncounterMatch.GetType().Name,
        homeTrackerFieldPresent = hasTrackerField,
        homeTrackerRequired = trackerRequired,
        homeTrackerPresent = hasTrackerField ? trackerPresent : (bool?)null,
        homeServerValidity = (string?)null,
        homeServerValidityNote = "Only Pokémon HOME can validate tracker/server history."
    }));

    return legality.Valid ? 0 : 3;
}
catch (Exception ex)
{
    Console.WriteLine(JsonSerializer.Serialize(new
    {
        parsed = false,
        valid = false,
        error = ex.GetType().Name,
        message = ex.Message
    }));
    return 2;
}
