using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using PKHeX.Core;

if (args.Length != 2)
{
    Console.Error.WriteLine("usage: LegacyBox.BatchGenerator <legacybox-manifest.json> <output-directory>");
    return 64;
}

var manifestPath = args[0];
var outputDirectory = args[1];
Directory.CreateDirectory(outputDirectory);

using var document = JsonDocument.Parse(File.ReadAllText(manifestPath));
var root = document.RootElement;
var trainerName = root.GetProperty("trainer").GetProperty("name").GetString() ?? "Doug";
var trainerLanguage = root.GetProperty("trainer").TryGetProperty("language", out var langNode)
    ? langNode.GetString() ?? "ENG"
    : "ENG";

if (!trainerLanguage.Equals("ENG", StringComparison.OrdinalIgnoreCase))
{
    Console.Error.WriteLine("initial batch generator currently supports ENG manifests only");
    return 65;
}

var receiver = new SimpleTrainerInfo(GameVersion.US)
{
    OT = trainerName,
    TID16 = StableTrainerID(trainerName, "USUM"),
    SID16 = StableTrainerID(trainerName, "USUM-SID"),
    Language = (int)LanguageID.English,
    Gender = 0,
    ConsoleRegion = 1,
    Country = 49,
    Region = 11,
};
RecentTrainerCache.SetRecentTrainer(receiver);

var results = new List<object>();
int passed = 0;
int failed = 0;

foreach (var entry in root.GetProperty("entries").EnumerateArray())
{
    ushort targetSpecies = checked((ushort)entry.GetProperty("national_dex").GetInt32());
    var speciesName = entry.GetProperty("species").GetString() ?? targetSpecies.ToString();
    byte targetLevel = checked((byte)entry.GetProperty("target_level").GetInt32());
    var templateNode = entry.GetProperty("template");
    var sourceGameText = templateNode.GetProperty("source_game").GetString() ?? string.Empty;
    var sourceVersion = GetVersion(sourceGameText);

    if (sourceVersion is not (GameVersion.RD or GameVersion.GN or GameVersion.YW))
    {
        results.Add(new
        {
            species = targetSpecies,
            name = speciesName,
            generated = false,
            legal = false,
            error = $"Initial batch generator only supports Gen I VC; source was {sourceGameText}.",
        });
        failed++;
        continue;
    }

    EntityConverter.VirtualConsoleSourceGen1 = sourceVersion;

    var sourceTrainer = new SimpleTrainerInfo(sourceVersion)
    {
        OT = trainerName,
        TID16 = StableTrainerID(trainerName, sourceGameText),
        SID16 = 0,
        Language = (int)LanguageID.English,
        Gender = 0,
    };

    var requestedShiny = GetRequestedShiny(entry);
    var criteria = new EncounterCriteria
    {
        Shiny = requestedShiny,
    };

    var generationTrainer = sourceTrainer;
    var catalogVersion = sourceVersion;
    var encounter = ResolveEncounter(targetSpecies, generationTrainer, catalogVersion);
    bool usedCanonicalGen1GiftFallback = false;

    // PKHeX intentionally collapses the Yellow Lv.10 Bulbasaur/Charmander/Squirtle
    // gifts into the shared Lv.5 RBY static templates when catch rate + initial moves
    // are equivalent. Preserve Yellow as the Transporter source, but use the shared
    // canonical static as the native legality seed when the YW-filtered lookup is empty.
    if (
        encounter is null
        && sourceVersion == GameVersion.YW
        && IsYellowCanonicalGiftFamily(templateNode)
    )
    {
        catalogVersion = GameVersion.RD;
        generationTrainer = new SimpleTrainerInfo(catalogVersion)
        {
            OT = trainerName,
            TID16 = sourceTrainer.TID16,
            SID16 = 0,
            Language = (int)LanguageID.English,
            Gender = 0,
        };
        encounter = ResolveEncounter(targetSpecies, generationTrainer, catalogVersion);
        usedCanonicalGen1GiftFallback = encounter is not null;
    }

    if (encounter is null)
    {
        results.Add(new
        {
            species = targetSpecies,
            name = speciesName,
            generated = false,
            legal = false,
            error = "No encounter candidate resolved.",
        });
        failed++;
        continue;
    }

    if (encounter.ConvertToPKM(generationTrainer, criteria) is not PK1 native)
    {
        results.Add(new
        {
            species = targetSpecies,
            name = speciesName,
            generated = false,
            legal = false,
            error = $"Encounter converted to {encounter.GetType().Name}, not PK1.",
        });
        failed++;
        continue;
    }

    native.Species = targetSpecies;
    native.CurrentLevel = targetLevel;

    var desiredNature = GetDesiredNature(entry);
    if (desiredNature is not Nature.Random)
        SetVCNature(native, targetSpecies, targetLevel, desiredNature);

    EncounterUtil.SetEncounterMoves(native, sourceVersion, targetLevel);
    native.HealPP();
    native.ResetPartyStats();

    // Re-assert the receiving 3DS trainer immediately before Transporter conversion.
    RecentTrainerCache.SetRecentTrainer(receiver);

    var desiredIVs = GetDesiredIVs(entry);
    var desiredHiddenPower = GetDesiredHiddenPower(entry);
    const int attempts = 50_000;
    PK7? best = null;
    int bestScore = int.MinValue;
    bool exactIVs = false;
    bool hiddenPowerMatch = desiredHiddenPower is null;

    for (int attempt = 0; attempt < attempts; attempt++)
    {
        var candidate = native.ConvertToPK7();
        var score = ScoreIVs(candidate, desiredIVs, desiredHiddenPower, out var ivExact, out var hpMatch);

        if (score > bestScore)
        {
            best = candidate;
            bestScore = score;
            exactIVs = ivExact;
            hiddenPowerMatch = hpMatch;
        }

        if (ivExact && hpMatch)
            break;
    }

    if (best is null)
    {
        results.Add(new
        {
            species = targetSpecies,
            name = speciesName,
            generated = false,
            legal = false,
            error = "Transporter conversion search produced no candidate.",
        });
        failed++;
        continue;
    }

    ApplyCompetitiveEVs(best, entry);
    ApplyCompetitiveMoves(best, entry);
    best.HealPP();
    best.RefreshChecksum();

    var legality = new LegalityAnalysis(best);
    var actualIVs = GetIVs(best);
    var actualEVs = GetEVs(best);
    var actualMoves = GetMoveNames(best);
    var actualHiddenPower = GetHiddenPowerName(best);
    var desiredNatureName = desiredNature is Nature.Random ? null : desiredNature.ToString();

    best.ForcePartyData();
    var bytes = new byte[best.SIZE_PARTY];
    best.WriteDecryptedDataParty(bytes);

    var fileName = $"{targetSpecies:000}_{Sanitize(speciesName)}.pk7";
    var filePath = Path.Combine(outputDirectory, fileName);
    File.WriteAllBytes(filePath, bytes);
    var sha256 = Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();

    var result = new
    {
        species = targetSpecies,
        name = speciesName,
        generated = true,
        legal = legality.Valid,
        encounterType = encounter.GetType().Name,
        originSpecies = encounter.Species,
        originLevelMin = encounter.LevelMin,
        originLevelMax = encounter.LevelMax,
        sourceVersion = sourceVersion.ToString(),
        catalogVersion = catalogVersion.ToString(),
        usedCanonicalGen1GiftFallback,
        ot = best.OriginalTrainerName,
        tid = best.TID16,
        level = best.CurrentLevel,
        shiny = best.IsShiny,
        nature = best.Nature.ToString(),
        desiredNature = desiredNatureName,
        natureMatch = desiredNature is Nature.Random || best.Nature == desiredNature,
        ability = GameInfo.Strings.abilitylist[best.Ability],
        actualIVs,
        desiredIVs,
        exactIVs,
        hiddenPower = actualHiddenPower,
        desiredHiddenPower,
        hiddenPowerMatch,
        actualEVs,
        moves = actualMoves,
        file = fileName,
        sha256,
    };
    results.Add(result);

    if (legality.Valid)
        passed++;
    else
        failed++;
}

var report = new
{
    manifest = Path.GetFileName(manifestPath),
    trainer = trainerName,
    total = passed + failed,
    passed,
    failed,
    allLegal = failed == 0,
    results,
};

var reportPath = Path.Combine(outputDirectory, "batch-report.json");
File.WriteAllText(
    reportPath,
    JsonSerializer.Serialize(report, new JsonSerializerOptions { WriteIndented = true })
);

Console.WriteLine(File.ReadAllText(reportPath));
return failed == 0 ? 0 : 3;


static IEncounterable? ResolveEncounter(
    ushort targetSpecies,
    ITrainerInfo trainer,
    GameVersion version)
{
    var rough = EntityBlank.GetBlank(trainer);
    rough.Species = targetSpecies;
    rough.Form = 0;
    rough.Gender = rough.GetSaneGender();

    return EncounterMovesetGenerator
        .GenerateEncounters(
            rough,
            trainer,
            ReadOnlyMemory<ushort>.Empty,
            version
        )
        .FirstOrDefault();
}

static bool IsYellowCanonicalGiftFamily(JsonElement templateNode)
{
    if (!templateNode.TryGetProperty("origin_national_dex", out var originNode))
        return false;

    return originNode.GetInt32() is 1 or 4 or 7;
}

static GameVersion GetVersion(string source)
{
    if (source.Contains("Red", StringComparison.OrdinalIgnoreCase))
        return GameVersion.RD;
    if (source.Contains("Blue", StringComparison.OrdinalIgnoreCase))
        return GameVersion.GN;
    if (source.Contains("Yellow", StringComparison.OrdinalIgnoreCase))
        return GameVersion.YW;
    return GameVersion.Invalid;
}

static ushort StableTrainerID(string trainer, string scope)
{
    var hash = SHA256.HashData(Encoding.UTF8.GetBytes($"{trainer}|{scope}"));
    var value = BitConverter.ToUInt16(hash, 0);
    return value == 0 ? (ushort)1 : value;
}

static Shiny GetRequestedShiny(JsonElement entry)
{
    if (!entry.TryGetProperty("traits", out var traits))
        return Shiny.Random;
    if (!traits.TryGetProperty("shiny", out var shinyNode))
        return Shiny.Random;
    return shinyNode.GetString() switch
    {
        "force_shiny" => Shiny.Always,
        "force_non_shiny" => Shiny.Never,
        _ => Shiny.Random,
    };
}

static Nature GetDesiredNature(JsonElement entry)
{
    if (!entry.TryGetProperty("competitive", out var comp))
        return Nature.Random;
    if (!comp.TryGetProperty("nature", out var natureNode))
        return Nature.Random;
    var value = natureNode.GetString();
    return value is not null && Enum.TryParse<Nature>(value, true, out var nature)
        ? nature
        : Nature.Random;
}

static void SetVCNature(PK1 pk, ushort species, byte level, Nature desired)
{
    var growth = PersonalTable.SM[species].EXPGrowth;
    uint min = Experience.GetEXP(level, growth);
    uint maxExclusive = level >= 100 ? min + 1 : Experience.GetEXP(checked((byte)(level + 1)), growth);

    for (uint exp = min; exp < maxExclusive; exp++)
    {
        if (Experience.GetNatureVC(exp) != desired)
            continue;
        pk.EXP = exp;
        return;
    }

    throw new InvalidOperationException(
        $"Could not realize VC nature {desired} for species {species} at level {level}."
    );
}

static Dictionary<string, int> GetDesiredIVs(JsonElement entry)
{
    var result = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
    if (!entry.TryGetProperty("competitive", out var comp))
        return result;
    if (!comp.TryGetProperty("ivs", out var ivs) || ivs.ValueKind != JsonValueKind.Object)
        return result;

    foreach (var prop in ivs.EnumerateObject())
        result[prop.Name] = prop.Value.GetInt32();
    return result;
}

static string? GetDesiredHiddenPower(JsonElement entry)
{
    if (!entry.TryGetProperty("competitive", out var comp))
        return null;
    if (!comp.TryGetProperty("moves", out var moves) || moves.ValueKind != JsonValueKind.Array)
        return null;

    foreach (var move in moves.EnumerateArray())
    {
        var name = move.GetString();
        if (name is null || !name.StartsWith("Hidden Power ", StringComparison.OrdinalIgnoreCase))
            continue;
        return name["Hidden Power ".Length..].Trim();
    }
    return null;
}

static int ScoreIVs(
    PK7 pk,
    Dictionary<string, int> desired,
    string? desiredHiddenPower,
    out bool exact,
    out bool hiddenPowerMatch)
{
    var actual = GetIVs(pk);
    exact = desired.Count == 0 || desired.All(kv =>
        actual.TryGetValue(kv.Key, out var v) && v == kv.Value
    );

    var actualHP = GetHiddenPowerName(pk);
    hiddenPowerMatch = desiredHiddenPower is null ||
        actualHP.Equals(desiredHiddenPower, StringComparison.OrdinalIgnoreCase);

    int score = hiddenPowerMatch ? 10_000 : 0;
    foreach (var (key, wanted) in desired)
    {
        if (!actual.TryGetValue(key, out var got))
            continue;
        if (got == wanted)
            score += 100;
        score -= Math.Abs(got - wanted);
    }
    return score;
}

static Dictionary<string, int> GetIVs(PKM pk)
{
    Span<int> values = stackalloc int[6];
    pk.GetIVs(values);
    // PKHeX order: HP, Atk, Def, Spe, SpA, SpD.
    return new Dictionary<string, int>
    {
        ["HP"] = values[0],
        ["Atk"] = values[1],
        ["Def"] = values[2],
        ["Spe"] = values[3],
        ["SpA"] = values[4],
        ["SpD"] = values[5],
    };
}

static Dictionary<string, int> GetEVs(PKM pk)
{
    Span<int> values = stackalloc int[6];
    pk.GetEVs(values);
    return new Dictionary<string, int>
    {
        ["HP"] = values[0],
        ["Atk"] = values[1],
        ["Def"] = values[2],
        ["Spe"] = values[3],
        ["SpA"] = values[4],
        ["SpD"] = values[5],
    };
}

static void ApplyCompetitiveEVs(PK7 pk, JsonElement entry)
{
    if (!entry.TryGetProperty("competitive", out var comp))
        return;
    if (!comp.TryGetProperty("evs", out var evs) || evs.ValueKind != JsonValueKind.Object)
        return;

    Span<int> values = stackalloc int[6];
    values.Clear();
    foreach (var prop in evs.EnumerateObject())
    {
        var value = prop.Value.GetInt32();
        switch (prop.Name.ToUpperInvariant())
        {
            case "HP":  values[0] = value; break;
            case "ATK": values[1] = value; break;
            case "DEF": values[2] = value; break;
            case "SPE": values[3] = value; break;
            case "SPA": values[4] = value; break;
            case "SPD": values[5] = value; break;
        }
    }
    pk.SetEVs(values);
}

static void ApplyCompetitiveMoves(PK7 pk, JsonElement entry)
{
    if (!entry.TryGetProperty("competitive", out var comp))
        return;
    if (!comp.TryGetProperty("moves", out var moves) || moves.ValueKind != JsonValueKind.Array)
        return;

    var names = GameInfo.Strings.movelist;
    Span<ushort> ids = stackalloc ushort[4];
    ids.Clear();
    int index = 0;
    foreach (var node in moves.EnumerateArray())
    {
        if (index == 4)
            break;
        var text = node.GetString();
        if (string.IsNullOrWhiteSpace(text))
            continue;
        var lookup = text.StartsWith("Hidden Power ", StringComparison.OrdinalIgnoreCase)
            ? "Hidden Power"
            : text;
        int move = Array.FindIndex(names, x => x.Equals(lookup, StringComparison.OrdinalIgnoreCase));
        if (move < 0)
            throw new InvalidOperationException($"Unknown move: {text}");
        ids[index++] = checked((ushort)move);
    }
    pk.SetMoves(ids);
}

static string[] GetMoveNames(PKM pk)
{
    var names = GameInfo.Strings.movelist;
    Span<ushort> moves = stackalloc ushort[4];
    pk.GetMoves(moves);
    return moves
        .ToArray()
        .Where(x => x != 0)
        .Select(x => names[x])
        .ToArray();
}

static string GetHiddenPowerName(PKM pk)
{
    var strings = GameInfo.Strings;
    return strings.Types[pk.HPType];
}

static string Sanitize(string value)
{
    var invalid = Path.GetInvalidFileNameChars();
    return new string(value.Select(ch => invalid.Contains(ch) ? '_' : ch).ToArray())
        .Replace(' ', '_');
}
