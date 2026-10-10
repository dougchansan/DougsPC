#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "legacybox.h"
#include "frontend.h"
#include "templates.h"
#include "box_ui.h"
#include "moon_save.h"

static LbxFrontend state;
static int tab, option, preset, checkpoint, seed_digit;
static bool loaded,verified;
static LbxBatch* batch;
static char message[160]="Choose Pokemon, then SELECT: prepare for Moon.";

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
    LbxView view={.state=&state,.visible=visible,.visible_count=visible_count,
        .tab=tab,.option=option,.preset=preset,.checkpoint=checkpoint,.seed_digit=seed_digit,
        .loaded=loaded,.verified=verified,.save_pending=lbx_moon_pending(),.batch_count=loaded?batch->count:0,.message=message};
    lbx_ui_render(&view);
}
static bool search_progress(int dex,int completed,int total,int attempts,int budget,int score,void* context) {
    uint64_t* last=context;
    if(!aptMainLoop()) return false;
    hidScanInput();
    if(hidKeysDown()&(KEY_B|KEY_START)) return false;
    uint64_t now=osGetTime();
    if(now-*last>=100 || attempts==0 || attempts==budget) {
        snprintf(message,sizeof(message),"Device IV search #%03d (%d/%d): %d/%d rolls. Best %d. B: cancel",
                 dex,completed+1,total,attempts,budget,attempts?score:0);
        render();gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();*last=now;
    }
    return true;
}
int main(void) {
    gfxInitDefault(); romfsInit(); lbx_ui_init();
    lbx_frontend_init(&state); state.rng_seed=(uint32_t)(svcGetSystemTick() ^ osGetTime()); filter(); batch=calloc(1,sizeof(*batch)); reload();
    int redraw=2;
    while(aptMainLoop()) {
        hidScanInput(); u32 down=hidKeysDown();
        if(down&KEY_START) {lbx_moon_discard();break;}
        if(lbx_moon_pending()) {
            if(down&KEY_TOUCH) {
                touchPosition p;hidTouchRead(&p);
                if(p.py>=110 && p.py<140)down|=p.px<160?KEY_A:KEY_B;
            }
            if(down&KEY_B) {lbx_moon_discard();snprintf(message,sizeof(message),"Preview cancelled. Moon unchanged; request kept.");}
            else if(down&KEY_A) {lbx_moon_commit(message,sizeof(message));}
            if(down)redraw=2;
            goto draw_frame;
        }
        if(down&KEY_L) {tab=(tab+4)%5;}
        if(down&KEY_R) {tab=(tab+1)%5;}
        if(down&KEY_TOUCH) {
            touchPosition p; hidTouchRead(&p);
            if(p.py<24) tab=p.px/64;
            else if(tab==0 && p.py>=26 && p.py<44) {
                int pages=(visible_count+29)/30;
                int page=state.cursor/30;
                if(p.px<50) page=(page+pages-1)%pages;
                else if(p.px>=270) page=(page+1)%pages;
                state.cursor=page*30;
            } else if(tab==0 && p.py>=48 && p.py<198) {
                int v=lbx_grid_hit(p.px,p.py,(state.cursor/30)*30,visible_count);
                if(v>=0) {state.cursor=v;state.selected[visible[v]]=!state.selected[visible[v]];}
            } else if(tab==0 && p.py>=204 && p.py<227) {
                if(p.px>=10 && p.px<155) down|=KEY_SELECT;
                else if(p.px>=165 && p.px<310) down|=KEY_X;
            } else if(tab==1 && p.py>=46 && p.py<197) {
                int row=(p.py-46)/22;
                if(row<7 && (p.py-46)%22<19) {option=row;down|=KEY_RIGHT;}
            } else if(tab==2 && p.py>=52 && p.py<182) {
                int row=(p.py-52)/34;
                if(row<4 && (p.py-52)%34<28) {preset=row;down|=KEY_A;}
            } else if(tab==3) {
                if(p.py>=88 && p.py<107) down|=KEY_A;
                else if(p.py>=120 && p.py<139) down|=KEY_X;
                else if(p.py>=152 && p.py<171) down|=KEY_Y;
            }
        }
        if(tab==0) {
            if(down&KEY_UP) state.cursor=lbx_grid_move(state.cursor,visible_count,0,-1);
            if(down&KEY_DOWN) state.cursor=lbx_grid_move(state.cursor,visible_count,0,1);
            if(down&KEY_LEFT) state.cursor=lbx_grid_move(state.cursor,visible_count,-1,0);
            if(down&KEY_RIGHT) state.cursor=lbx_grid_move(state.cursor,visible_count,1,0);
            if(down&KEY_A) state.selected[visible[state.cursor]]=!state.selected[visible[state.cursor]];
            if(down&KEY_B) memset(state.selected,0,sizeof(state.selected));
            if(down&KEY_Y) {state.generation=(state.generation+1)%8;state.cursor=0;filter();}
            if(down&KEY_SELECT) {
                mkdir(LBX_ROOT,0777);LbxSaveIO io;uint64_t last_progress=0;
                if(lbx_moon_archive(&io,message,sizeof(message)) &&
                   lbx_moon_prepare(&state,&io,search_progress,&last_progress,message,sizeof(message))) tab=3;
            }
            if(down&KEY_X) {
                mkdir(LBX_ROOT,0777);
                uint64_t last_progress=0;
                if(lbx_save_request_progress(&state,LBX_REQUEST,message,sizeof(message),search_progress,&last_progress)) snprintf(message,sizeof(message),state.rng_mode==2?"Device search saved. PC builds/checks Pokemon files.":"Saved request.tsv. Process with PC helper.");
            }
        } else if(tab==1) {
            if(down&KEY_UP) option=(option+6)%7;
            if(down&KEY_DOWN) option=(option+1)%7;
            int delta=(down&KEY_RIGHT)?1:(down&KEY_LEFT)?-1:0;
            if(option==5) {
                if(down&KEY_Y) state.rng_seed=(uint32_t)(svcGetSystemTick() ^ osGetTime());
                if(down&KEY_X) seed_digit=(seed_digit+1)%8;
                unsigned shift=(unsigned)(7-seed_digit)*4;
                unsigned nibble=((state.rng_seed>>shift)+delta+16)&15u;
                state.rng_seed=(state.rng_seed&~(15u<<shift))|(nibble<<shift);
            } else {
                int* fields[]={&state.level,&state.nature,&state.shiny,&state.start_box,&state.rng_mode,NULL,&state.rng_budget};
                int limits[]={101,26,3,32,3,1,3};
                *fields[option]=(*fields[option]+delta+limits[option])%limits[option];
            }
        } else if(tab==2) {
            if(down&KEY_UP) preset=(preset+3)%4;
            if(down&KEY_DOWN) preset=(preset+1)%4;
            if(down&KEY_A) {lbx_preset(&state,preset);snprintf(message,sizeof(message),"Preset selected; review batch settings.");}
        } else if(tab==3) {
            if(down&KEY_Y) reload();
            if(loaded && (down&KEY_X)) {
                LbxSaveIO io;
                if(lbx_moon_archive(&io,message,sizeof(message)))lbx_moon_prepare_batch(&io,message,sizeof(message));
            }
            if(loaded && (down&(KEY_A|KEY_SELECT))) {
                int count=0;
                verified=lbx_verify_batch(batch,&count,message,sizeof(message));
                if(!verified) remove(LBX_READY_FLAG);
                if(verified) snprintf(message,sizeof(message),"PASS: %d/%d SHA256. Integrity only.",count,batch->count);
                if(verified && (down&KEY_SELECT) && lbx_write_ready_flag(batch,message,sizeof(message))) {
                    if(!save_queue(checkpoint>1?checkpoint:1))snprintf(message,sizeof(message),"READY written; queue persistence failed.");
                    else snprintf(message,sizeof(message),"PKSM fallback READY; run injector and save in PKSM.");
                }

            }
        } else if(tab==4 && loaded) {
            if(down&KEY_A) {
                if(checkpoint<1) snprintf(message,sizeof(message),"Stage verified batch first (Batch tab).");
                else if(checkpoint<4 && !save_queue(checkpoint+1)) snprintf(message,sizeof(message),"Could not save confirmation.");
            }
            if(down&KEY_B) {if(!save_queue(0)) snprintf(message,sizeof(message),"Could not reset confirmations.");}
        }
draw_frame:
        if(down) redraw=2;
        if(redraw>0) {render();redraw--;}
        gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
    }
    lbx_moon_discard();free(batch);lbx_ui_exit();romfsExit();gfxExit();return 0;
}

