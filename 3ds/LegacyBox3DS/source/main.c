#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "legacybox.h"
#include "frontend.h"
#include "templates.h"

static const char* tabs[]={"Browse","Options","Presets","Batch","Queue"};
static const char* steps[]={"Staged for PKSM","Save/readback confirmed","Bank box confirmed","HOME receipt confirmed"};
static const char* natures[]={"Template","Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"};
static PrintConsole top,bottom;
static LbxFrontend state;
static int tab, option, preset, checkpoint, seed_digit;
static bool loaded,verified;
static LbxBatch* batch;
static char message[160]="Choose Pokemon, then save a request.";

static void load_queue(void) {
    checkpoint=0;
    if(!loaded) return;
    FILE* f=fopen(LBX_ROOT "/queue.tsv","rb");
    char id[65]; int n=0;
    if(f) { if(fscanf(f,"LBXQ1\t%64s\t%d",id,&n)==2 && !strcmp(id,batch->batch_id) && n>=0 && n<=4) checkpoint=n; fclose(f); }
}
static bool save_queue(int next) {
    FILE* f=fopen(LBX_ROOT "/queue.tsv.tmp","wb");
    if(!f) return false;
    bool ok=fprintf(f,"LBXQ1\t%s\t%d\n",batch->batch_id,next)>0;
    if(fclose(f)!=0) ok=false;
    if(ok && rename(LBX_ROOT "/queue.tsv.tmp",LBX_ROOT "/queue.tsv")==0) {checkpoint=next; return true;}
    remove(LBX_ROOT "/queue.tsv.tmp"); return false;
}
static void reload(void) {
    verified=false;
    loaded=batch && lbx_load_batch(batch,message,sizeof(message));
    load_queue();
    if(loaded) snprintf(message,sizeof(message),"Batch loaded; verify before staging.");
}
static int visible[LBX_DEX_COUNT],visible_count;
static void filter(void) {
    visible_count=0;
    for(int i=0;i<LBX_DEX_COUNT;i++) if(!state.generation || lbx_generations[i]==state.generation) visible[visible_count++]=i;
    if(state.cursor>=visible_count) state.cursor=0;
}
static void render(void) {
    consoleSelect(&top); consoleClear();
    printf("LegacyBox - Preserve. Validate. Migrate.\n\n");
    printf("%s | Selected: %d\n",tabs[tab],lbx_selected_count(&state));
    printf("Target: Box %d onward\n\n",state.start_box+1);
    if(visible_count) { int d=visible[state.cursor]; printf("#%03d %s (Gen %d)\n",d+1,lbx_species[d],lbx_generations[d]); }
    printf("\n%s\n\n",message);
    printf("Offline assembly preserves reviewed template bytes.\n");
    printf("Existing batches can be verified and staged.\n");
    printf("Bank/HOME confirmations are manual.\n");
    printf("\nL/R: tab   START: exit\n");
    consoleSelect(&bottom); consoleClear();
    printf("Browse  Options Presets Batch   Queue\n");
    printf("----------------------------------------\n");
    if(tab==0) {
        printf("Gen: %s (Y cycles filter)\n",state.generation ? "filtered":"all");
        int first=(state.cursor/8)*8;
        for(int row=0;row<8;row++) {
            int v=first+row;
            printf("\x1b[%d;1H",7+row*2);
            if(v<visible_count) {int d=visible[v]; printf("%c [%c] #%03d %.25s",v==state.cursor?'>':' ',state.selected[d]?'x':' ',d+1,lbx_species[d]);}
        }
        printf("\x1b[25;1HA: toggle B: clear SELECT: assemble\n");
        printf("\x1b[28;1HX: Save generation request");
    } else if(tab==1) {
        printf("Batch settings (0 = preserve template)\n\n");
        printf("%c Level: %d\n\n",option==0?'>':' ',state.level);
        printf("%c Nature: %s\n\n",option==1?'>':' ',natures[state.nature]);
        printf("%c Shiny: %s\n\n",option==2?'>':' ',(const char*[]){"Template","Request shiny","Non-shiny"}[state.shiny]);
        printf("%c Start box: %d\n\n",option==3?'>':' ',state.start_box+1);
        printf("%c RNG: %s\n\n",option==4?'>':' ',state.rng_mode?"Fixed transfer seed":"Default random");
        printf("%c Seed: %08lX (digit %d)\n\n",option==5?'>':' ',(unsigned long)state.rng_seed,seed_digit+1);
        printf("%c IV search: %s rolls\n\n",option==6?'>':' ',(const char*[]){"5,000","50,000","250,000"}[state.rng_budget]);
        printf("Up/Down: field; Left/Right: change\n");
        printf("X: seed digit. RNG applies on PC.\n");
        printf("Fixed events keep distribution settings.\n");
        printf("IV/EV/moves use reviewed template.\n");
    } else if(tab==2) {
        printf("A applies highlighted preset\n\n");
        const char* names[]={"Heritage #001-009","Living Dex request #001-807","Shiny heritage request","Clear selection"};
        for(int i=0;i<4;i++) printf("%c %s\n\n",preset==i?'>':' ',names[i]);
        printf("Shiny requests require reviewed eligibility.\n");
        printf("Events/origin sampler: templates pending.\n");
    } else if(tab==3) {
        printf("Prepared batch: %s\n",loaded?"loaded":"not found");
        if(loaded) printf("%.16s... | %d files\n",batch->batch_id,batch->count);
        printf("\nA: Verify SHA256\n\nX: Reverify and mark READY\n\nY: Reload batch from SD\n\n");
        printf("Integrity: %s\n",verified?"PASS":"unchecked");
        printf("Generation request does not replace batch.\n");
    } else {
        printf("Manual evidence checklist for this batch\n\n");
        for(int i=0;i<4;i++) printf("[%c] %s\n\n",checkpoint>i?'x':' ',steps[i]);
        printf("A: Confirm next observed step\n\nB: Reset confirmations\n\n");
        printf("Save/readback, Bank, HOME need observation.\n");
        printf("Rosalina automation: hardware probe pending.");
    }
}
int main(void) {
    gfxInitDefault(); consoleInit(GFX_TOP,&top); consoleInit(GFX_BOTTOM,&bottom);
    lbx_frontend_init(&state); filter(); batch=calloc(1,sizeof(*batch)); reload();
    bool dirty=true;
    while(aptMainLoop()) {
        hidScanInput(); u32 down=hidKeysDown();
        if(down&KEY_START) break;
        if(down&KEY_L) {tab=(tab+4)%5; dirty=true;}
        if(down&KEY_R) {tab=(tab+1)%5; dirty=true;}
        if(down&KEY_TOUCH) {
            touchPosition p; hidTouchRead(&p);
            if(p.py<16) tab=p.px/64;
            else if(tab==0 && p.py>=48 && p.py<176) {
                int v=(state.cursor/8)*8+(p.py-48)/16;
                if(v<visible_count) {state.cursor=v; state.selected[visible[v]]=!state.selected[visible[v]];}
            } else if(tab==0 && p.py>=184 && p.py<208) down|=KEY_SELECT;
            else if(tab==0 && p.py>=208) down|=KEY_X;
            else if(tab==1 && p.py>=40 && p.py<152) {option=(p.py-40)/16;down|=KEY_RIGHT;}
            else if(tab==2 && p.py>=40 && p.py<104) {preset=(p.py-40)/16;down|=KEY_A;}
            dirty=true;
        }
        if(tab==0) {
            if(down&KEY_UP) state.cursor=(state.cursor+visible_count-1)%visible_count;
            if(down&KEY_DOWN) state.cursor=(state.cursor+1)%visible_count;
            if(down&KEY_LEFT) state.cursor=(state.cursor+visible_count-8)%visible_count;
            if(down&KEY_RIGHT) state.cursor=(state.cursor+8)%visible_count;
            if(down&KEY_A) state.selected[visible[state.cursor]]=!state.selected[visible[state.cursor]];
            if(down&KEY_B) memset(state.selected,0,sizeof(state.selected));
            if(down&KEY_Y) {state.generation=(state.generation+1)%8;state.cursor=0;filter();}
            if(down&KEY_SELECT) {
                if(lbx_assemble_templates(&state,message,sizeof(message))) {
                    char result[160]; snprintf(result,sizeof(result),"%s",message);
                    reload(); snprintf(message,sizeof(message),"%s",result); tab=3;
                }
            }
            if(down&KEY_X) {
                mkdir(LBX_ROOT,0777);
                if(lbx_save_request(&state,LBX_REQUEST,message,sizeof(message))) snprintf(message,sizeof(message),"Saved request.tsv. Process with PC helper.");
            }
        } else if(tab==1) {
            if(down&KEY_UP) option=(option+6)%7;
            if(down&KEY_DOWN) option=(option+1)%7;
            int delta=(down&KEY_RIGHT)?1:(down&KEY_LEFT)?-1:0;
            if(option==5) {
                if(down&KEY_X) seed_digit=(seed_digit+1)%8;
                unsigned shift=(unsigned)(7-seed_digit)*4;
                unsigned nibble=((state.rng_seed>>shift)+delta+16)&15u;
                state.rng_seed=(state.rng_seed&~(15u<<shift))|(nibble<<shift);
            } else {
                int* fields[]={&state.level,&state.nature,&state.shiny,&state.start_box,&state.rng_mode,NULL,&state.rng_budget};
                int limits[]={101,26,3,32,2,1,3};
                *fields[option]=(*fields[option]+delta+limits[option])%limits[option];
            }
        } else if(tab==2) {
            if(down&KEY_UP) preset=(preset+3)%4;
            if(down&KEY_DOWN) preset=(preset+1)%4;
            if(down&KEY_A) {lbx_preset(&state,preset);snprintf(message,sizeof(message),"Preset selected; review batch settings.");}
        } else if(tab==3) {
            if(down&KEY_Y) reload();
            if(loaded && (down&(KEY_A|KEY_X))) {
                int count=0;
                verified=lbx_verify_batch(batch,&count,message,sizeof(message));
                if(!verified) remove(LBX_READY_FLAG);
                if(verified) snprintf(message,sizeof(message),"PASS: %d/%d SHA256. Integrity only.",count,batch->count);
                if(verified && (down&KEY_X)) {
                    if(lbx_write_ready_flag(batch,message,sizeof(message))) {
                        if(!save_queue(checkpoint>1?checkpoint:1)) snprintf(message,sizeof(message),"READY written; queue persistence failed.");
                        else snprintf(message,sizeof(message),"READY. Run PKSM, save, then verify readback.");
                    }
                }
            }
        } else if(tab==4 && loaded) {
            if(down&KEY_A) {
                if(checkpoint<1) snprintf(message,sizeof(message),"Stage verified batch first (Batch tab).");
                else if(checkpoint<4 && !save_queue(checkpoint+1)) snprintf(message,sizeof(message),"Could not save confirmation.");
            }
            if(down&KEY_B) {if(!save_queue(0)) snprintf(message,sizeof(message),"Could not reset confirmations.");}
        }
        if(down) dirty=true;
        if(dirty) {render();dirty=false;}
        gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
    }
    free(batch);gfxExit();return 0;
}
