#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "legacybox.h"

static void draw_header(void)
{
    consoleClear();
    printf("\x1b[1;1HLegacyBox3DS - Bank Stager\n");
    printf("--------------------------------\n");
    printf("A  Verify current batch\n");
    printf("X  Mark verified batch ready for PKSM\n");
    printf("START  Exit to HOME Menu\n\n");
    printf("Bank network/auth remains in the official app.\n\n");
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    char error[160] = {0};
    LbxBatch* batch = calloc(1, sizeof(*batch));
    bool loaded = false;
    bool verified = false;
    int verified_count = 0;

    if (!batch) {
        snprintf(error, sizeof(error), "Out of memory allocating batch state.");
    } else {
        loaded = lbx_load_batch(batch, error, sizeof(error));
    }

    draw_header();

    if (loaded) {
        printf("Batch: %.16s...\n", batch->batch_id);
        printf("Files: %d\n", batch->count);
        printf("Target: Box %d onward\n", batch->start_box + 1);
    } else {
        printf("No usable batch: %s\n", error);
        printf("Expected: %s\n", LBX_MANIFEST);
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if ((down & KEY_A) && loaded) {
            draw_header();
            printf("Verifying %d files...\n", batch->count);
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();

            memset(error, 0, sizeof(error));
            verified = lbx_verify_batch(batch, &verified_count, error, sizeof(error));
            if (verified)
                printf("PASS: %d/%d files match SHA256.\n", verified_count, batch->count);
            else
                printf("FAIL after %d files: %s\n", verified_count, error);
        }

        if ((down & KEY_X) && loaded) {
            draw_header();
            if (!verified) {
                printf("Verify with A first. Nothing staged.\n");
            } else {
                memset(error, 0, sizeof(error));
                if (lbx_write_ready_flag(batch, error, sizeof(error))) {
                    printf("Batch marked READY.\n\n");
                    printf("Next:\n");
                    printf("1. Exit LegacyBox3DS.\n");
                    printf("2. Open PKSM with your USUM save.\n");
                    printf("3. Run LegacyBox Batch Injector.\n");
                    printf("4. Exit PKSM normally to commit the save.\n");
                    printf("5. Open official Pokemon Bank.\n");
                } else {
                    printf("Could not stage batch: %s\n", error);
                }
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    free(batch);
    gfxExit();
    return 0;
}
