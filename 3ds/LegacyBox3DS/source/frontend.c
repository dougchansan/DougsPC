#include "frontend.h"
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
bool lbx_save_request(const LbxFrontend* s, const char* path, char* error, size_t size) {
    int count=lbx_selected_count(s);
    if(!count || s->level<0 || s->level>100 || s->nature<0 || s->nature>25 ||
       s->shiny<0 || s->shiny>2 || s->start_box<0 || s->start_box>31 ||
       count>960-s->start_box*30 || s->rng_mode<0 || s->rng_mode>1 || s->rng_budget<0 || s->rng_budget>2) {
        snprintf(error,size,"Empty request or invalid options/capacity."); return false;
    }
    char temp[256];
    if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp)) {
        snprintf(error,size,"Request path too long."); return false;
    }
    FILE* f=fopen(temp,"wb");
    if(!f) { snprintf(error,size,"Cannot write request on SD."); return false; }
    bool ok=fprintf(f,"LBXR2\t%d\t%d\t%d\t%d\t%d\t%d\t%08lX\t%d\n",count,s->start_box,s->level,s->nature,s->shiny,s->rng_mode,(unsigned long)s->rng_seed,s->rng_budget)>0;
    for(int i=0;i<LBX_DEX_COUNT && ok;i++) if(s->selected[i]) ok=fprintf(f,"%d\n",i+1)>0;
    if(fclose(f)!=0) ok=false;
    if(ok && rename(temp,path)==0) return true;
    remove(temp); snprintf(error,size,"Request write/rename failed."); return false;
}
