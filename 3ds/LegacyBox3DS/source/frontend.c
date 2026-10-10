#include "frontend.h"
#include "search_rng.h"
#include "device_profiles.inc"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "catalog.inc"
void lbx_frontend_init(LbxFrontend* s) { memset(s, 0, sizeof(*s)); s->rng_budget=1; s->rng_seed=0x4C425831u; }
int lbx_selected_count(const LbxFrontend* s) {
    int n=0; for(int i=0;i<LBX_DEX_COUNT;i++) n += s->selected[i]; return n;
}
void lbx_preset(LbxFrontend* s, int preset) {
    memset(s->selected, 0, sizeof(s->selected));
    if(preset==0) for(int i=0;i<9;i++) s->selected[i]=true;
    if(preset==1) for(int i=0;i<LBX_DEX_COUNT;i++) s->selected[i]=true;
    if(preset==2) { for(int i=0;i<9;i++) s->selected[i]=true; s->shiny=1; }
}
static const LbxSearchProfile* profile_for(int dex) {
    for(size_t i=0;i<sizeof(device_profiles)/sizeof(device_profiles[0]);i++)
        if(device_profiles[i].dex==dex) return &device_profiles[i];
    return NULL;
}
bool lbx_save_request_progress(const LbxFrontend* s,const char* path,char* error,size_t size,
                               LbxSearchProgress progress,void* context) {
    int count=lbx_selected_count(s);
    if(!count || s->level<0 || s->level>100 || s->nature<0 || s->nature>25 ||
       s->shiny<0 || s->shiny>2 || s->start_box<0 || s->start_box>31 ||
       count>960-s->start_box*30 || s->rng_mode<0 || s->rng_mode>2 || s->rng_budget<0 || s->rng_budget>2) {
        snprintf(error,size,"Empty request or invalid options/capacity."); return false;
    }
    if(s->rng_mode==2) for(int i=151;i<LBX_DEX_COUNT;i++) if(s->selected[i]) {
        snprintf(error,size,"Device rolls: Gen I VC species 1-151 only."); return false;
    }
    if(s->rng_mode==2) for(int i=0;i<LBX_DEX_COUNT;i++) if(s->selected[i] && !profile_for(i+1)) {
        snprintf(error,size,"No reviewed device search profile for #%03d.",i+1); return false;
    }
    char temp[256];
    if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp)) {
        snprintf(error,size,"Request path too long."); return false;
    }
    FILE* f=fopen(temp,"wb");
    if(!f) { snprintf(error,size,"Cannot write request on SD."); return false; }
    bool ok=fprintf(f,"%s\t%d\t%d\t%d\t%d\t%d\t%d\t%08lX\t%d\n",s->rng_mode==2?"LBXR4":"LBXR2",count,s->start_box,s->level,s->nature,s->shiny,s->rng_mode,(unsigned long)s->rng_seed,s->rng_budget)>0;
    int completed=0;bool cancelled=false;
    for(int i=0;i<LBX_DEX_COUNT && ok;i++) if(s->selected[i]) {
        if(s->rng_mode==2) {
            const LbxSearchProfile* profile=profile_for(i+1);
            LbxSearch search;
            lbx_search_init(&search,s->rng_seed,profile,(int[]){5000,50000,250000}[s->rng_budget]);
            do {
                if(progress && !progress(i+1,completed,count,search.attempts,search.budget,search.score,context)) {
                    cancelled=true;ok=false;break;
                }
                lbx_search_step(&search,256);
            } while(!search.done);
            if(!ok) break;
            if(progress && !progress(i+1,completed,count,search.attempts,search.budget,search.score,context)) {
                cancelled=true;ok=false;break;
            }
            LbxLocalRoll roll=search.best;
            ok=fprintf(f,"%d\t%08lX\t%08lX\t%08lX\t%d\t%d\t%s\n",i+1,
                (unsigned long)roll.ec,(unsigned long)roll.pid,(unsigned long)roll.ivs,
                search.attempts,search.best_attempt,profile->hash)>0;
            completed++;
        } else ok=fprintf(f,"%d\n",i+1)>0;
    }
    if(fclose(f)!=0) ok=false;
    if(ok && rename(temp,path)==0) return true;
    remove(temp); snprintf(error,size,cancelled?"Search cancelled. Previous request preserved.":"Request write/rename failed."); return false;
}


bool lbx_save_request(const LbxFrontend* s,const char* path,char* error,size_t size) {
    return lbx_save_request_progress(s,path,error,size,NULL,NULL);
}
