#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "box_ui.h"

/* CPU framebuffer renderer: no extra graphics library, no network in the UI. */
#define RGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
static const unsigned BG=RGB(15,27,44), PANEL=RGB(27,45,66), CELL=RGB(37,61,81);
static const unsigned INK=RGB(231,242,246), MUTED=RGB(150,180,197);
static const unsigned TEAL=RGB(32,182,159), GOLD=RGB(255,210,92);
static unsigned char* fb;
static int width;
static FILE* packs[2];
static unsigned char icons[30][40*30*4];
static bool found[30];
static int cache_first=-1,cache_gen=-1,cache_shiny=-1;
static const char* tabs[]={"Browse","Options","Presets","Batch","Queue"};
static const char* steps[]={"Staged for PKSM","Save/readback checked","Bank deposit observed","HOME receipt observed"};
static const char* natures[]={"Template","Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"};
/* Original five-column glyphs, one byte per column; bit 0 is the top. */
static const unsigned char font[][5]={
 {62,17,17,17,62},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {7,8,112,8,7},{97,81,73,69,67},
 {62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},
 {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
 {54,73,73,73,54},{6,73,73,41,30}
};
static void pixel(int x,int y,unsigned color) {
    if(x<0 || x>=width || y<0 || y>=240) return;
    unsigned char* p=fb+((x*240)+(239-y))*3;
    p[0]=color&255;p[1]=(color>>8)&255;p[2]=(color>>16)&255;
}
static void rect(int x,int y,int w,int h,unsigned color) {
    for(int a=x;a<x+w;a++) for(int b=y;b<y+h;b++) pixel(a,b,color);
}
static void outline(int x,int y,int w,int h,unsigned color) {
    rect(x,y,w,2,color);rect(x,y+h-2,w,2,color);rect(x,y,2,h,color);rect(x+w-2,y,2,h,color);
}
static void text(int x,int y,const char* s,unsigned color,int scale) {
    int start=x;
    for(;*s;s++) {
        unsigned char c=(unsigned char)toupper((unsigned char)*s),glyph[5]={0};
        if(c=='\n'){x=start;y+=9*scale;continue;}
        if(c>='A'&&c<='Z') memcpy(glyph,font[c-'A'],5);
        else if(c>='0'&&c<='9') memcpy(glyph,font[26+c-'0'],5);
        else if(c=='-') memset(glyph,8,5);
        else if(c=='_') memset(glyph,64,5);
        else if(c=='\'') glyph[2]=3;
        else if(c=='.') glyph[2]=64;
        else if(c==':') glyph[2]=20;
        else if(c=='/') {glyph[0]=64;glyph[1]=32;glyph[2]=24;glyph[3]=4;glyph[4]=2;}
        else if(c=='#') {glyph[0]=20;glyph[1]=127;glyph[2]=20;glyph[3]=127;glyph[4]=20;}
        else if(c=='<') {glyph[1]=8;glyph[2]=20;glyph[3]=34;}
        else if(c=='>') {glyph[1]=34;glyph[2]=20;glyph[3]=8;}
        else if(c=='+') {glyph[1]=8;glyph[2]=28;glyph[3]=8;}
        else if(c=='?') {glyph[0]=2;glyph[1]=1;glyph[2]=81;glyph[3]=9;glyph[4]=6;}
        for(int i=0;i<5;i++) for(int j=0;j<7;j++) if(glyph[i]&(1<<j)) rect(x+i*scale,y+j*scale,scale,scale,color);
        x+=6*scale;
    }
}
static void wrapped(int x,int y,const char* s,int columns,unsigned color) {
    char line[68];int n=0;
    while(*s && y<230) {
        line[n++]=*s++;
        if(n==columns || !*s) {line[n]=0;text(x,y,line,color,1);y+=10;n=0;}
    }
}
static FILE* open_pack(const char* path) {
    FILE* f=fopen(path,"rb");unsigned char h[16];
    if(!f)return NULL;
    long expected=16L+807L*40*30*4;
    if(fread(h,1,16,f)!=16 || memcmp(h,"LBXS1\0\0\0",8) ||
       h[8]!=40 || h[9]!=0 || h[10]!=30 || h[11]!=0 ||
       h[12]!=39 || h[13]!=3 || h[14]!=0 || h[15]!=0 ||
       fseek(f,0,SEEK_END) || ftell(f)!=expected) {fclose(f);return NULL;}
    return f;
}
void lbx_ui_init(void) {
    packs[0]=open_pack("/3ds/LegacyBox/assets/regular.bin");
    packs[1]=open_pack("/3ds/LegacyBox/assets/shiny.bin");
}
void lbx_ui_exit(void) {for(int i=0;i<2;i++)if(packs[i])fclose(packs[i]);}
static void cache(const LbxView* v) {
    int first=(v->state->cursor/30)*30,shiny=v->state->shiny==1;
    if(first==cache_first && cache_gen==v->state->generation && shiny==cache_shiny)return;
    cache_first=first;cache_gen=v->state->generation;cache_shiny=shiny;
    for(int i=0;i<30;i++) {
        found[i]=false;
        if(first+i>=v->visible_count || !packs[shiny])continue;
        int dex=v->visible[first+i];
        if(fseek(packs[shiny],16L+dex*(long)sizeof(icons[i]),SEEK_SET)==0)
            found[i]=fread(icons[i],1,sizeof(icons[i]),packs[shiny])==sizeof(icons[i]);
    }
}
static void sprite(int index,int x,int y,int scale) {
    if(index<0 || index>=30 || !found[index]) {text(x+8,y+8,"?",MUTED,scale);return;}
    for(int j=0;j<30;j++)for(int i=0;i<40;i++) {
        const unsigned char* p=&icons[index][(j*40+i)*4];
        if(p[3]>=128)rect(x+i*scale,y+j*scale,scale,scale,RGB(p[0],p[1],p[2]));
    }
}
static void screen(gfxScreen_t screen,int w) {
    width=w;fb=gfxGetFramebuffer(screen,GFX_LEFT,NULL,NULL);rect(0,0,w,240,BG);
}
static void card(int y,const char* label,const char* value,bool active) {
    rect(10,y,300,19,active?CELL:PANEL);
    if(active)rect(10,y,3,19,TEAL);
    text(18,y+6,label,MUTED,1);text(120,y+6,value,active?GOLD:INK,1);
}
void lbx_ui_render(const LbxView* v) {
    cache(v);const LbxFrontend* s=v->state;char line[100];
    screen(GFX_TOP,400);
    rect(0,0,400,34,PANEL);text(14,10,"DOUG'S PC",INK,2);text(235,14,"PRESERVE / MIGRATE",TEAL,1);
    snprintf(line,sizeof(line),"%d SELECTED / BOX %d ONWARD",lbx_selected_count(s),s->start_box+1);text(14,47,line,TEAL,1);
    if(v->visible_count) {
        int d=v->visible[s->cursor];
        snprintf(line,sizeof(line),"#%03d",d+1);text(14,69,line,GOLD,2);
        text(14,94,lbx_species[d],INK,strlen(lbx_species[d])>17?1:2);
        snprintf(line,sizeof(line),"GENERATION %d",lbx_generations[d]);text(14,116,line,MUTED,1);
        text(14,133,s->selected[d]?"SELECTED FOR MOON":"A / TOUCH TO SELECT",TEAL,1);
        rect(256,62,130,100,PANEL);sprite(s->cursor%30,261,67,3);
    }
    rect(10,170,380,46,PANEL);wrapped(18,178,v->message,60,INK);
    text(14,225,"CATALOG PREVIEW / NOT LIVE SAVE CONTENTS",MUTED,1);
    screen(GFX_BOTTOM,320);
    for(int i=0;i<5;i++) {
        rect(i*64,0,64,24,i==v->tab?TEAL:PANEL);
        text(i*64+5,9,tabs[i],i==v->tab?BG:MUTED,1);
        if(i==v->tab)rect(i*64,22,64,2,GOLD);
    }
    if(v->save_pending) {
        text(12,40,"REVIEW MOON SAVE",GOLD,2);
        text(12,67,"TARGET SLOTS CHECKED EMPTY",TEAL,1);
        text(12,83,"BOX 8 PROTECTED / BACKUP BEFORE WRITE",MUTED,1);
        rect(10,110,145,30,TEAL);text(20,122,"A: SAVE TO MOON",BG,1);
        rect(165,110,145,30,PANEL);text(180,122,"B: CANCEL",INK,1);
        text(12,161,"SAVE INCLUDES COMMIT + FULL READBACK",MUTED,1);
        text(12,185,"AFTER SUCCESS: EXIT AND OPEN BANK",TEAL,1);
        text(12,209,"PKSM-CORE / GPLV3+ - SEE SOURCE NOTICES",MUTED,1);
        return;
    }
    if(v->tab==0) {
        int first=(s->cursor/30)*30;
        snprintf(line,sizeof(line),"CATALOG %d/%d - GEN %s",first/30+1,(v->visible_count+29)/30,s->generation?"FILTER":"ALL");
        text(20,33,"<",GOLD,1);text(55,33,line,INK,1);text(287,33,">",GOLD,1);
        for(int i=0;i<30;i++) {
            int x=10+(i%6)*50,y=48+(i/6)*30,index=first+i;
            rect(x,y,48,28,index<v->visible_count?CELL:PANEL);
            if(index>=v->visible_count)continue;
            int d=v->visible[index];sprite(i,x+4,y-1,1);
            if(s->selected[d]) {rect(x+39,y+3,6,6,TEAL);text(x+38,y+2,"+",BG,1);}
            if(index==s->cursor)outline(x,y,48,28,GOLD);
        }
        rect(10,204,145,23,TEAL);text(17,212,"SELECT: MOON",BG,1);
        rect(165,204,145,23,PANEL);text(172,212,"X: SAVE REQUEST",INK,1);
        text(12,232,"Y: FILTER  B: CLEAR  L/R: TABS",MUTED,1);
    } else if(v->tab==1) {
        text(12,31,"BATCH OPTIONS / TEMPLATE = PRESERVE",MUTED,1);
        snprintf(line,sizeof(line),"%d",s->level);card(46,"LEVEL",s->level?line:"Preserve",v->option==0);
        card(68,"NATURE",natures[s->nature],v->option==1);
        card(90,"SHINY",(const char*[]){"Template","Request shiny","Non-shiny"}[s->shiny],v->option==2);
        snprintf(line,sizeof(line),"%d",s->start_box+1);card(112,"START BOX",line,v->option==3);
        card(134,"RNG",(const char*[]){"PC default","PC fixed seed","Device search"}[s->rng_mode],v->option==4);
        snprintf(line,sizeof(line),"%08lX / DIGIT %d",(unsigned long)s->rng_seed,v->seed_digit+1);card(156,"SEED",line,v->option==5);
        card(178,"IV SEARCH",(const char*[]){"5000","50000","250000"}[s->rng_budget],v->option==6);
        text(12,208,"UP/DOWN: FIELD  LEFT/RIGHT: CHANGE",MUTED,1);
        text(12,222,s->rng_mode==2?"X: DIGIT Y: NEW SEED / REVIEWED RECIPES":"LEVEL: PRESERVE OR 1-100 / X: SEED DIGIT",MUTED,1);
    } else if(v->tab==2) {
        const char* names[]={"Heritage #001-009","Living Dex #001-807","Shiny heritage request","Clear selection"};
        text(12,33,"PRESETS / A OR TOUCH TO APPLY",MUTED,1);
        for(int i=0;i<4;i++) {rect(10,52+i*34,300,28,i==v->preset?CELL:PANEL);text(20,62+i*34,names[i],i==v->preset?GOLD:INK,1);}
        text(12,202,"REVIEW OPTIONS AFTER APPLYING",TEAL,1);
        text(12,218,"MISSING TEMPLATES BLOCK ASSEMBLY",MUTED,1);
    } else if(v->tab==3) {
        snprintf(line,sizeof(line),"%s / %d FILES",v->loaded?"BATCH LOADED":"NO BATCH",v->batch_count);text(12,35,line,INK,1);
        card(54,"INTEGRITY",v->verified?"PASS":"Unchecked",false);
        card(88,"A","Verify SHA256",true);card(120,"X","Prepare for Moon",false);card(152,"Y","Reload from SD",false);
        text(12,199,"X: PREVIEW THEN BACKUP + SAVE TO MOON",TEAL,1);
        text(12,216,"SELECT: PKSM FALLBACK READY",MUTED,1);
    } else {
        text(12,35,"OBSERVED TRANSFER CHECKLIST",MUTED,1);
        for(int i=0;i<4;i++) {rect(10,54+i*32,300,25,PANEL);rect(18,61+i*32,10,10,v->checkpoint>i?TEAL:CELL);text(36,63+i*32,steps[i],v->checkpoint>i?INK:MUTED,1);}
        text(12,198,"A: CONFIRM OBSERVED NEXT STEP",TEAL,1);
        text(12,213,"B: RESET / HOME MOVING KEY MANUAL",MUTED,1);
    }
}
