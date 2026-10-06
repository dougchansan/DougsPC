#include <pksm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LegacyBox PKSM injector v0.3 -- PicoC-compatible paths and pointer checks. */
/* Keep each path a single literal: PicoC cannot concatenate ROOT + a literal. */
#define ROOT "/3ds/LegacyBox/inbox/current"
#define MANIFEST "/3ds/LegacyBox/inbox/current/batch.tsv"
#define READY "/3ds/LegacyBox/ready.flag"
#define MAX_LINE 512
#define MAX_FILE 96
#define MAX_COUNT 900

struct row
{
    char filename[MAX_FILE];
    int box;
    int slot;
};

static int parse_index(char* text)
{
    int value = 0;
    int i = 0;
    if (text[0] == '\0') return -1;
    while (text[i] != '\0')
    {
        if (text[i] < '0' || text[i] > '9') return -1;
        value = value * 10 + text[i] - '0';
        if (value > 1000) return -1;
        i++;
    }
    return value;
}

static int parse_row(char* line, struct row* out)
{
    char* p = strchr(line, '\t');
    if (p == NULL) return 0;
    *p = '\0';
    int length = strlen(line);
    if (length < 5 || length >= MAX_FILE) return 0;
    if (strchr(line, '/') != NULL) return 0;
    if (strchr(line, '\\') != NULL) return 0;
    if (strstr(line, "..") != NULL) return 0;
    if (strcmp(&line[length - 4], ".pk7") != 0) return 0;
    strcpy(out->filename, line);

    char* sha = p + 1;
    p = strchr(sha, '\t');
    if (p == NULL) return 0;
    *p = '\0';
    if (strlen(sha) != 64) return 0;
    for (int i = 0; i < 64; i++)
    {
        if (strchr("0123456789abcdef", sha[i]) == NULL) return 0;
    }

    char* box = p + 1;
    p = strchr(box, '\t');
    if (p == NULL) return 0;
    *p = '\0';
    char* slot = p + 1;
    char* nl = strchr(slot, '\n');
    if (nl != NULL) *nl = '\0';
    nl = strchr(slot, '\r');
    if (nl != NULL) *nl = '\0';

    out->box = parse_index(box);
    out->slot = parse_index(slot);
    return out->box >= 0 && out->slot >= 0 && out->slot < 30;
}

static int load_rows(struct row* rows, int* count, char* batchID)
{
    FILE* manifest = fopen(MANIFEST, "rb");
    if (manifest == NULL) return 0;
    char line[MAX_LINE];
    /* Do not negate a pointer-returning function call in PicoC. */
    char* lineRead = fgets(line, MAX_LINE, manifest);
    if (lineRead == NULL)
    {
        fclose(manifest);
        return 0;
    }
    char magic[8];
    int startBox = -1;
    int fields = sscanf(line, "%7[^\t]\t%64[^\t]\t%d\t%d", magic, batchID, count, &startBox);
    if (fields != 4 || strcmp(magic, "LBX1") != 0 || *count < 1 || *count > MAX_COUNT || startBox < 0)
    {
        fclose(manifest);
        return 0;
    }
    if (strlen(batchID) != 64)
    {
        fclose(manifest);
        return 0;
    }
    for (int i = 0; i < *count; i++)
    {
        lineRead = fgets(line, MAX_LINE, manifest);
        if (lineRead == NULL || !parse_row(line, &rows[i]))
        {
            fclose(manifest);
            return 0;
        }
        for (int j = 0; j < i; j++)
        {
            if (rows[i].box == rows[j].box && rows[i].slot == rows[j].slot)
            {
                fclose(manifest);
                return 0;
            }
        }
    }
    /* Extra rows must not be silently ignored. */
    lineRead = fgets(line, MAX_LINE, manifest);
    fclose(manifest);
    return lineRead == NULL;
}

static int ready_matches(char* batchID, int count)
{
    FILE* f = fopen(READY, "rb");
    if (f == NULL) return 0;
    char magic[8];
    char readyID[65];
    int readyCount = 0;
    int fields = fscanf(f, "%7s\n%64s\n%d", magic, readyID, &readyCount);
    fclose(f);
    if (fields != 3) return 0;
    return strcmp(magic, "LBX1") == 0 && strcmp(readyID, batchID) == 0 && readyCount == count;
}

static int read_pk7(char* filename, char* out, int size)
{
    char path[256];
    snprintf(path, 256, "%s/%s", ROOT, filename);
    FILE* f = fopen(path, "rb");
    if (f == NULL) return 0;
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return 0;
    }
    long length = ftell(f);
    if (length != 232 && length != 260)
    {
        fclose(f);
        return 0;
    }
    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return 0;
    }
    int got = fread(out, 1, size, f);
    fclose(f);
    return got == size;
}

int main(int argc, char** argv)
{
    /* PKSM supplies the loaded save's version byte through argv[0]. */
    if (argc < 1 || argv == NULL)
    {
        gui_warn("Run this script from PKSM with a Sun/Moon or USUM save loaded.");
        return 0;
    }
    if (argv[0] == NULL) return 0;
    unsigned char version = *argv[0];
    if (version != 30 && version != 31 && version != 32 && version != 33)
    {
        gui_warn("LegacyBox v0.3: select a Sun/Moon or Ultra Sun/Ultra Moon save. No changes made.");
        return 0;
    }

    struct row* rows = malloc(sizeof(struct row) * MAX_COUNT);
    if (rows == NULL)
    {
        gui_warn("Out of memory. No changes made.");
        return 0;
    }
    int count = 0;
    char batchID[65];
    if (!load_rows(rows, &count, batchID))
    {
        free(rows);
        gui_warn("Invalid or missing LegacyBox batch.tsv. No changes made.");
        return 0;
    }
    if (!ready_matches(batchID, count))
    {
        free(rows);
        gui_warn("Batch not READY or READY belongs to another batch. Run LegacyBox3DS A then X again.");
        return 0;
    }

    int boxSize = pkx_box_size(GEN_SEVEN);
    if (boxSize != 232)
    {
        free(rows);
        gui_warn("Unexpected Gen VII storage size. No changes made.");
        return 0;
    }
    char* existing = malloc(boxSize);
    char* data = malloc(boxSize * count);
    if (existing == NULL || data == NULL)
    {
        free(existing);
        free(data);
        free(rows);
        gui_warn("Out of memory. No changes made.");
        return 0;
    }

    int maxBoxes = sav_get_max(MAX_BOXES);
    sav_box_decrypt();
    int preflightOK = 1;
    char* problem = "Preflight failed. No changes made.";
    for (int i = 0; i < count; i++)
    {
        if (rows[i].box >= maxBoxes)
        {
            problem = "Target box does not exist. No changes made.";
            preflightOK = 0;
            break;
        }
        /* Cache the whole batch before the first mutation. No later SD re-reads. */
        char* pk = &data[i * boxSize];
        if (!read_pk7(rows[i].filename, pk, boxSize))
        {
            problem = "Missing, unreadable or wrong-sized PK7. No changes made.";
            preflightOK = 0;
            break;
        }
        sav_get_pkx(existing, rows[i].box, rows[i].slot);
        if (pkx_get_value(existing, GEN_SEVEN, SPECIES) != 0)
        {
            problem = "A target PC slot is occupied. No changes made.";
            preflightOK = 0;
            break;
        }
        if (!pkx_is_valid(pk, GEN_SEVEN))
        {
            problem = "A PK7 failed PKSM structural checks. No changes made.";
            preflightOK = 0;
            break;
        }
    }
    if (!preflightOK)
    {
        sav_box_encrypt();
        free(existing);
        free(data);
        free(rows);
        gui_warn(problem);
        return 0;
    }
    if (!gui_choice("LegacyBox v0.3: preflight passed. Back up this save first. Inject into the empty target slots?"))
    {
        sav_box_encrypt();
        free(existing);
        free(data);
        free(rows);
        return 0;
    }

    int writeOK = 1;
    for (int i = 0; i < count; i++)
    {
        char* pk = &data[i * boxSize];
        sav_inject_pkx(pk, GEN_SEVEN, rows[i].box, rows[i].slot, 0);
        sav_get_pkx(existing, rows[i].box, rows[i].slot);
        if (memcmp(existing, pk, boxSize) != 0)
        {
            writeOK = 0;
            break;
        }
    }
    sav_box_encrypt();
    free(existing);
    free(data);
    free(rows);
    if (!writeOK)
    {
        gui_warn("Read-back mismatch. DO NOT SAVE. Discard PKSM changes and reload your backup before retrying.");
        return 0;
    }
    gui_warn("LegacyBox v0.3: batch staged and read-back checked. Save in PKSM, then check the boxes in-game. Bank transfer is still manual.");
    return 0;
}
