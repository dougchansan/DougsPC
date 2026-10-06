#include <pksm.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOT "/3ds/LegacyBox/inbox/current"
#define MANIFEST ROOT "/batch.tsv"
#define READY "/3ds/LegacyBox/ready.flag"
#define MAX_LINE 512
#define MAX_FILE 96

struct row
{
    char filename[MAX_FILE];
    int box;
    int slot;
};

static int parse_row(char* line, struct row* out)
{
    char* p = strchr(line, '\t');
    if (!p) return 0;
    *p = '\0';

    if (strlen(line) >= MAX_FILE) return 0;
    strcpy(out->filename, line);

    // Skip SHA-256.
    char* sha = p + 1;
    p = strchr(sha, '\t');
    if (!p) return 0;
    *p = '\0';

    char* box = p + 1;
    p = strchr(box, '\t');
    if (!p) return 0;
    *p = '\0';

    char* slot = p + 1;
    char* nl = strchr(slot, '\n');
    if (nl) *nl = '\0';

    out->box = atoi(box);
    out->slot = atoi(slot);
    return out->box >= 0 && out->slot >= 0 && out->slot < 30;
}

static int read_header(FILE* manifest, int* count)
{
    char line[MAX_LINE];
    if (!fgets(line, sizeof(line), manifest)) return 0;

    char magic[8] = {0};
    char batch[80] = {0};
    int startBox = 0;
    if (sscanf(line, "%7[^\t]\t%79[^\t]\t%d\t%d", magic, batch, count, &startBox) != 4)
        return 0;

    return !strcmp(magic, "LBX1") && *count > 0 && *count <= 900;
}

static int load_rows(struct row* rows, int* count)
{
    FILE* manifest = fopen(MANIFEST, "rb");
    if (!manifest) return 0;

    if (!read_header(manifest, count))
    {
        fclose(manifest);
        return 0;
    }

    char line[MAX_LINE];
    for (int i = 0; i < *count; i++)
    {
        if (!fgets(line, sizeof(line), manifest) || !parse_row(line, &rows[i]))
        {
            fclose(manifest);
            return 0;
        }
    }

    fclose(manifest);
    return 1;
}

static int file_exists(char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int read_pk7(char* filename, char* out, int size)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", ROOT, filename);

    FILE* f = fopen(path, "rb");
    if (!f) return 0;

    int read = fread(out, 1, size, f);
    fclose(f);
    return read == size;
}

int main(int argc, char** argv)
{
    // PicoC rejects casts to void; leave unused entry-point arguments alone.

    if (!file_exists(READY))
    {
        gui_warn("LegacyBox batch is not marked READY. Verify it in LegacyBox3DS first.");
        return 0;
    }

    struct row* rows = malloc(sizeof(struct row) * 900);
    if (!rows)
    {
        gui_warn("Out of memory.");
        return 0;
    }

    int count = 0;
    if (!load_rows(rows, &count))
    {
        free(rows);
        gui_warn("Could not parse /3ds/LegacyBox/inbox/current/batch.tsv");
        return 0;
    }

    int boxSize = pkx_box_size(GEN_SEVEN);
    char* existing = malloc(boxSize);
    char* data = malloc(boxSize);
    if (!existing || !data)
    {
        free(existing);
        free(data);
        free(rows);
        gui_warn("Out of memory.");
        return 0;
    }

    int maxBoxes = sav_get_max(MAX_BOXES);

    sav_box_decrypt();

    // Phase 1: preflight ALL slots and files. No writes occur here.
    for (int i = 0; i < count; i++)
    {
        if (rows[i].box >= maxBoxes)
        {
            sav_box_encrypt();
            free(existing);
            free(data);
            free(rows);
            gui_warn("Batch targets a box that does not exist in this save.");
            return 0;
        }

        if (!read_pk7(rows[i].filename, data, boxSize))
        {
            sav_box_encrypt();
            free(existing);
            free(data);
            free(rows);
            gui_warn("Could not read a PK7 from the verified LegacyBox batch.");
            return 0;
        }

        sav_get_pkx(existing, rows[i].box, rows[i].slot);
        if (pkx_get_value(existing, GEN_SEVEN, SPECIES) != 0)
        {
            sav_box_encrypt();
            free(existing);
            free(data);
            free(rows);
            gui_warn("A target PC slot is occupied. LegacyBox will not overwrite it.");
            return 0;
        }

        if (!pkx_is_valid(data, GEN_SEVEN))
        {
            sav_box_encrypt();
            free(existing);
            free(data);
            free(rows);
            gui_warn("A staged PK7 failed PKSM's local structural validation.");
            return 0;
        }
    }

    // Phase 2: all preflight checks passed, inject sequentially.
    for (int i = 0; i < count; i++)
    {
        read_pk7(rows[i].filename, data, boxSize);
        sav_inject_pkx(data, GEN_SEVEN, rows[i].box, rows[i].slot, 0);
    }

    sav_box_encrypt();

    free(existing);
    free(data);
    free(rows);

    gui_splash("LegacyBox batch injected into the loaded save. Exit PKSM normally to commit the save, then open official Pokemon Bank.");
    return 0;
}
