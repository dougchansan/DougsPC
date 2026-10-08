using System.Buffers.Binary;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using PKHeX.Core;

// An independent deterministic sampler for requested VC transfer fields.
// This is not an implementation of original hardware encounter RNG.
internal sealed record TransferRngSettings(string Mode, string? SeedHex, int SearchAttempts)
{
    public static TransferRngSettings Read(JsonElement root)
    {
        if (!root.TryGetProperty("rng", out var node)) return new("default", null, 50_000);
        if (node.GetProperty("scope").GetString() != "vc_transfer_rolls")
            throw new ArgumentException("Unsupported RNG scope");
        var mode = node.GetProperty("mode").GetString();
        var attempts = node.GetProperty("search_attempts").GetInt32();
        if (mode is not ("default" or "fixed") || attempts is < 1 or > 250_000)
            throw new ArgumentException("Invalid RNG mode/search budget");
        string? hex = mode == "fixed" ? node.GetProperty("seed_hex").GetString() : null;
        if (mode == "fixed" && (hex is null || hex.Length != 8 ||
            !uint.TryParse(hex, NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out _)))
            throw new ArgumentException("Fixed RNG seed must be eight hex digits");
        return new(mode!, hex?.ToUpperInvariant(), attempts);
    }
    public ulong? SeedFor(ushort species, string templateId)
    {
        if (Mode != "fixed") return null;
        var bytes = SHA256.HashData(Encoding.UTF8.GetBytes($"LBX-VC-ROLLS-1|{SeedHex}|{species.ToString(CultureInfo.InvariantCulture)}|{templateId}"));
        return BinaryPrimitives.ReadUInt64LittleEndian(bytes);
    }
}

internal sealed class TransferRolls(ulong seed)
{
    private ulong state = seed;
    private uint Next()
    {
        unchecked
        {
            state += 0x9E3779B97F4A7C15UL;
            ulong z = state;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9UL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBUL;
            return (uint)((z ^ (z >> 31)) >> 32);
        }
    }
    private int Index(int bound)
    {
        uint limit = unchecked(0u - (uint)bound) % (uint)bound;
        uint n;
        do { n = Next(); } while (n < limit);
        return (int)(n % (uint)bound);
    }
    public void Apply(PK7 candidate, bool sourceShiny)
    {
        candidate.EncryptionConstant = Next();
        uint pid = Next();
        uint low = pid & 0xFFFF;
        uint xor = (pid >> 16) ^ low ^ candidate.TID16 ^ candidate.SID16;
        if (sourceShiny) pid = ((low ^ candidate.TID16 ^ candidate.SID16) << 16) | low;
        else if (xor < 16) pid ^= 1u << 28;
        candidate.PID = pid;
        Span<int> order = stackalloc int[] { 0, 1, 2, 3, 4, 5 };
        for (int i = 5; i > 0; i--)
        {
            int j = Index(i + 1);
            int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
        Span<int> ivs = stackalloc int[6];
        for (int i = 0; i < 6; i++) ivs[i] = Index(32);
        int guaranteed = candidate.Species == 151 ? 5 : 3;
        for (int i = 0; i < guaranteed; i++) ivs[order[i]] = 31;
        candidate.IV_HP = ivs[0]; candidate.IV_ATK = ivs[1]; candidate.IV_DEF = ivs[2];
        candidate.IV_SPE = ivs[3]; candidate.IV_SPA = ivs[4]; candidate.IV_SPD = ivs[5];
        candidate.RefreshChecksum();
    }
}
