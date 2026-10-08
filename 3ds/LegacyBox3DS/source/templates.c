#include "templates.h"
#include "legacybox.h"
#include "sha256.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#define LIBRARY LBX_ROOT "/templates"
#define NEW_BATCH LBX_ROOT "/inbox/frontend-new"
#define OLD_BATCH LBX_ROOT "/inbox/frontend-backup"
typedef struct {int dex;char name[96],hash[65];} Template;
static bool exists(const char* p) {struct stat st;return stat(p,&st)==0;}
static bool safe_name(const char* s) {
    if(!*s || strstr(s,"..")) return false;
    for(;*s;s++) if(!isalnum((unsigned char)*s) && *s!='_' && *s!='-' && *s!='.') return false;
    return true;
}
bool lbx_assemble_templates(const LbxFrontend* s,char* error,size_t size) {
    bool ok=false;Template* entries=NULL;FILE* f=NULL;FILE* out=NULL;
    int count=lbx_selected_count(s),total=0,found=0;
    snprintf(error,size,"Offline: template fields and default RNG only.");
    if(s->level || s->nature || s->shiny || s->rng_mode || s->rng_budget!=1) return false;
    snprintf(error,size,"Empty selection or box capacity exceeded.");
    if(!count || s->start_box<0 || s->start_box>31 || count>960-s->start_box*30) return false;
    snprintf(error,size,"Install a reviewed templates pack on SD first.");
    f=fopen(LIBRARY "/templates.tsv","rb");if(!f) return false;
    char line[256];
    if(!fgets(line,sizeof(line),f) || sscanf(line,"LBXT1\t%d",&total)!=1 || total<1 || total>807) goto done;
    entries=calloc((size_t)total,sizeof(*entries));if(!entries) goto done;
    bool seen[807]={false};
    for(int i=0;i<total;i++) {
        Template* t=&entries[i];int used=0;
        if(!fgets(line,sizeof(line),f) || sscanf(line,"%d\t%95[^\t]\t%64[^\r\n]%n",&t->dex,t->name,t->hash,&used)!=3 ||
           t->dex<1 || t->dex>807 || seen[t->dex-1] || !safe_name(t->name) || strlen(t->hash)!=64 ||
           (line[used]!='\r' && line[used]!='\n' && line[used]!=0)) goto done;
        for(int j=0;j<64;j++) if(!strchr("0123456789abcdef",t->hash[j])) goto done;
        for(int j=0;j<i;j++) if(!strcmp(entries[j].name,t->name)) goto done;
        seen[t->dex-1]=true;
        if(s->selected[t->dex-1]) found++;
    }
    fclose(f);f=NULL;
    snprintf(error,size,"Some selected species have no installed template.");
    if(found!=count) goto done;
    snprintf(error,size,"Resolve frontend-new/backup folders before assembling again.");
    if(exists(NEW_BATCH) || exists(OLD_BATCH)) goto done;
    if(mkdir(LBX_ROOT "/inbox",0777)!=0 && errno!=EEXIST) goto done;
    if(mkdir(NEW_BATCH,0777)!=0) goto done;
    LbxSha256 id; lbx_sha256_init(&id);int index=0;
    for(int i=0;i<total;i++) if(s->selected[entries[i].dex-1]) {
        Template* t=&entries[i];char path[256],actual[65],material[256];uint8_t data[261],digest[32];
        snprintf(path,sizeof(path),"%s/%s",LIBRARY,t->name);
        f=fopen(path,"rb");if(!f) goto done;
        size_t n=fread(data,1,sizeof(data),f);bool read_ok=!ferror(f);fclose(f);f=NULL;
        snprintf(error,size,"Template bytes failed integrity check.");
        if(!read_ok || (n!=232 && n!=260)) goto done;
        LbxSha256 hash;lbx_sha256_init(&hash);lbx_sha256_update(&hash,data,n);lbx_sha256_final(&hash,digest);lbx_sha256_hex(digest,actual);
        if(strcmp(actual,t->hash)) goto done;
        snprintf(path,sizeof(path),"%s/%s",NEW_BATCH,t->name);
        out=fopen(path,"wb");if(!out) goto done;
        bool wrote=fwrite(data,1,n,out)==n;if(fclose(out)!=0) wrote=false;
    out=NULL;
    if(!wrote) goto done;
        int len=snprintf(material,sizeof(material),"%s%s:%s:%d:%d",index?"\n":"",t->name,t->hash,s->start_box+index/30,index%30);
        lbx_sha256_update(&id,(uint8_t*)material,(size_t)len);index++;
    }
    uint8_t digest[32];char batch_id[65];lbx_sha256_final(&id,digest);lbx_sha256_hex(digest,batch_id);
    out=fopen(NEW_BATCH "/batch.tsv","wb");if(!out) goto done;
    bool wrote=fprintf(out,"LBX1\t%s\t%d\t%d\n",batch_id,count,s->start_box)>0;index=0;
    for(int i=0;i<total && wrote;i++) if(s->selected[entries[i].dex-1]) {
        wrote=fprintf(out,"%s\t%s\t%d\t%d\n",entries[i].name,entries[i].hash,s->start_box+index/30,index%30)>0;index++;
    }
    if(fclose(out)!=0) wrote=false;
    out=NULL;
    if(!wrote) goto done;
    snprintf(error,size,"Cannot invalidate stale ready flag.");
    if(remove(LBX_READY_FLAG)!=0 && errno!=ENOENT) goto done;
    bool had_current=exists(LBX_INBOX);
    snprintf(error,size,"Could not preserve/replace current batch.");
    if(had_current && rename(LBX_INBOX,OLD_BATCH)!=0) goto done;
    if(rename(NEW_BATCH,LBX_INBOX)!=0) {if(had_current) rename(OLD_BATCH,LBX_INBOX);goto done;}
    ok=true;snprintf(error,size,"Assembled %d exact templates. Verify in Batch tab.",count);
done:
    if(f) fclose(f);
    if(out) fclose(out);
    free(entries);return ok;
}
