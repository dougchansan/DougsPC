using PKHeX.Core;
var paths = Directory.GetFiles(args[0], "*.pk7");
if (paths.Length != 36) throw new InvalidOperationException($"Expected 36 native readback Pokémon, got {paths.Length}");
foreach (var path in paths)
{
    var pk = new PK7(File.ReadAllBytes(path));
    var la = new LegalityAnalysis(pk);
    if (!pk.ChecksumValid || !la.Valid)
        throw new InvalidOperationException($"{Path.GetFileName(path)}: {la.Report()}");
    var variant = int.Parse(Path.GetFileName(path).Split('-')[0]);
    if (pk.HandlingTrainerName != "Kento") throw new InvalidOperationException("Receiving trainer not applied");
    if (variant == 2 && (!pk.IsShiny || pk.CurrentLevel != 100 || pk.Nature != Nature.Modest)) throw new InvalidOperationException("Shiny/level/nature options not applied");
    if (variant == 3 && (pk.IsShiny || pk.CurrentLevel != 100 || pk.Nature != Nature.Adamant)) throw new InvalidOperationException("Non-shiny/level/nature options not applied");
}
foreach (var path in Directory.GetFiles(args[0], "*.main"))
{
    var save = new SAV7SM(File.ReadAllBytes(path));
    if (!save.ChecksumsValid) throw new InvalidOperationException($"Native Moon checksums failed: {path}");
    for (int i = 0; i < 9; i++)
    {
        var pk = save.GetBoxSlotAtIndex(1, i);
        if (pk.Species != i + 1 || !new LegalityAnalysis(pk).Valid) throw new InvalidOperationException($"Native encrypted Moon slot {i} failed");
    }
}
Console.WriteLine("PASS: 36 native saved/readback Pokémon passed independent PKHeX legality, and all four Moon saves passed checksums and encrypted-slot parsing.");
