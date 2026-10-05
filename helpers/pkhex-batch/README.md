# PKHeX batch generator

This helper is the first concrete generation proof for LegacyBox manifests.

Current scope: **Generation I 3DS Virtual Console -> Gen VII PK7**.

For each entry it:

1. asks PKHeX.Core to resolve the selected Gen I encounter/evolution origin;
2. generates a PK1 with OT Doug and a deterministic per-origin TID;
3. applies requested shiny state when legal;
4. evolves/levels the native representation to the manifest target;
5. realizes the competitive Nature through the actual VC Experience-modulo-25 rule;
6. converts through PKHeX's Poké Transporter logic into PK7;
7. searches Transporter RNG outputs for the best match to the preferred competitive IV spread;
8. applies the preferred EV spread and legal Gen VII competitive moves;
9. runs a full PKHeX legality analysis;
10. writes the concrete .pk7 and a SHA-256 audit report.

The helper never fabricates Bank/HOME metadata or a HOME tracker.

Exact preferred IVs are **not** forced after conversion. If Transporter RNG does not produce the requested spread within the search budget, the best legal result is retained and the report records `exactIVs: false`.
