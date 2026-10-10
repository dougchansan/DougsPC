// Doug's PC Moon adapter. Uses unmodified GPLv3+ PKSM-Core; see THIRD_PARTY.md.
#include "moon_save.h"
extern "C" {
#include "legacybox.h"
#include "sha256.h"
}
#include "search_rng.h"
#include "device_profiles.inc"
#include "sav/SavSUMO.hpp"
#include "pkx/PK7.hpp"
#include <array>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <sys/stat.h>
#include <unistd.h>

namespace {
using Bytes=std::vector<uint8_t>;
struct Pending { Bytes before, after; LbxSaveIO io; int count=0,first=0,last=0; };
std::unique_ptr<Pending> pending;
struct Candidate { std::unique_ptr<pksm::PK7> pk; int box,slot; };
void hash(const uint8_t* data,size_t size,char out[65]) {
    LbxSha256 h;uint8_t digest[32];lbx_sha256_init(&h);lbx_sha256_update(&h,data,size);
    lbx_sha256_final(&h,digest);lbx_sha256_hex(digest,out);
}
bool read_file(const char* path,Bytes& out,size_t limit) {
    FILE* f=fopen(path,"rb");if(!f)return false;
    out.resize(limit+1);size_t n=fread(out.data(),1,out.size(),f);bool ok=!ferror(f);
    fclose(f);out.resize(n);return ok && n<=limit;
}
bool safe_name(const char* s) {
    if(!*s || strstr(s,".."))return false;
    for(;*s;s++)if(!isalnum((unsigned char)*s)&&*s!='_'&&*s!='-'&&*s!='.')return false;
    return true;
}
bool valid_pk(pksm::PK7& pk) {
    auto checksum=pk.checksum();pk.refreshChecksum();
    return pk.sanity()==0 && pk.checksum()==checksum && pk.species()!=pksm::Species::None &&
           u16(pk.species())<=802 && !pk.egg();
}
std::shared_ptr<uint8_t[]> copy_data(const Bytes& bytes) {
    auto data=std::shared_ptr<uint8_t[]>(new uint8_t[bytes.size()]);
    memcpy(data.get(),bytes.data(),bytes.size());return data;
}
bool stage(std::vector<Candidate>& candidates,const LbxSaveIO* io,char* error,size_t size) {
    if(!io || !io->read || !io->write || candidates.empty())return false;
    auto p=std::make_unique<Pending>();p->io=*io;p->before.resize(LBX_MOON_SAVE_SIZE);
    if(!io->read(p->before.data(),p->before.size(),error,size,io->context))return false;
    auto data=copy_data(p->before);pksm::SavSUMO save(data);
    if(save.version()!=pksm::GameVersion::MN || !save.checksumsValid()) {
        snprintf(error,size,"Moon save identity/checksums failed. No edits.");return false;
    }
    bool used[32][30]{};
    for(auto& c:candidates) {
        if(c.box<0 || c.box>=32 || c.slot<0 || c.slot>=30 || c.box==7 || used[c.box][c.slot]) {
            snprintf(error,size,"Invalid/duplicate target or protected Box 8.");return false;
        }
        used[c.box][c.slot]=true;
        auto old=save.pkm((u8)c.box,(u8)c.slot);
        if(!old || old->species()!=pksm::Species::None) {
            snprintf(error,size,"Box %d slot %d occupied. Nothing written.",c.box+1,c.slot+1);return false;
        }
        if(!valid_pk(*c.pk)) {snprintf(error,size,"Pokemon structure/checksum failed.");return false;}
    }
    for(auto& c:candidates) {
        // Match PKSM's receiving-trainer handling, encrypt stored slots, update dex.
        save.pkm(*c.pk,(u8)c.box,(u8)c.slot,true);save.dex(*c.pk);
        auto readback=save.pkm((u8)c.box,(u8)c.slot);
        auto expected=c.pk->clone();save.trade(*expected);
        expected->refreshChecksum();
        if(!readback || memcmp(readback->rawData().data(),expected->rawData().data(),232)) {
            snprintf(error,size,"In-memory Pokemon readback failed.");return false;
        }
    }
    save.finishEditing();
    if(!save.checksumsValid()) {snprintf(error,size,"Moon resign/checksum failed.");return false;}
    p->after.assign(data.get(),data.get()+LBX_MOON_SAVE_SIZE);
    p->count=(int)candidates.size();p->first=candidates.front().box+1;p->last=candidates.back().box+1;
    pending=std::move(p);
    snprintf(error,size,"%s preview: %d Pokemon -> Box %d-%d. A: backup + save. B: cancel.",
             io->label?io->label:"Moon",pending->count,pending->first,pending->last);return true;
}
bool backup(const Bytes& bytes,char* error,size_t size) {
    char digest[65],path[200];hash(bytes.data(),bytes.size(),digest);
    if(mkdir(LBX_ROOT "/backups",0777) && errno!=EEXIST)return false;
    snprintf(path,sizeof(path),LBX_ROOT "/backups/moon-%s.main",digest);
    Bytes existing;
    if(read_file(path,existing,bytes.size())) {
        if(existing==bytes)return true;
        snprintf(error,size,"Existing backup differs. Save blocked.");return false;
    }
    FILE* f=fopen(path,"wbx");if(!f)return false;
    bool ok=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size() && fflush(f)==0;
    if(ok)ok=fsync(fileno(f))==0;
    if(fclose(f))ok=false;
    Bytes checked;
    if(!ok || !read_file(path,checked,bytes.size()) || checked!=bytes) {
        snprintf(error,size,"Backup write/readback failed. Game unchanged.");return false;
    }
    return true;
}
}
extern "C" void lbx_moon_discard(void) {pending.reset();}
extern "C" bool lbx_moon_pending(void) {return !!pending;}
static bool prepare_batch(const LbxSaveIO* io,char* error,size_t size) {
    pending.reset();auto batch=std::make_unique<LbxBatch>();int verified=0;
    if(!lbx_load_batch(batch.get(),error,size) || !lbx_verify_batch(batch.get(),&verified,error,size))return false;
    std::vector<Candidate> candidates;
    for(int i=0;i<batch->count;i++) {
        auto& e=batch->entries[i];Bytes bytes;char path[256];snprintf(path,sizeof(path),LBX_INBOX "/%s",e.filename);
        if(!read_file(path,bytes,260) || (bytes.size()!=232 && bytes.size()!=260)) {
            snprintf(error,size,"Cannot load complete PK7 batch.");return false;
        }
        auto pk=pksm::PKX::getPKM<pksm::Generation::SEVEN>(bytes.data(),bytes.size());
        if(!pk || !valid_pk(*pk)) {snprintf(error,size,"Invalid PK7. No edits.");return false;}
        candidates.push_back({std::move(pk),e.box,e.slot});
    }
    return stage(candidates,io,error,size);
}
static bool prepare_selected(const LbxFrontend* s,const LbxSaveIO* io,LbxSearchProgress progress,
                                 void* context,char* error,size_t size) {
    pending.reset();
    if(!s || !lbx_selected_count(s)) {snprintf(error,size,"Select Pokemon first.");return false;}
    if(s->rng_mode==1 || (s->rng_mode!=2 && (s->level||s->nature||s->shiny))) {
        snprintf(error,size,"For local changes choose Device search. PC RNG exports only.");return false;
    }
    // Search + request export first; cancellation never writes the game.
    if(!lbx_save_request_progress(s,LBX_REQUEST,error,size,progress,context))return false;
    std::array<LbxLocalRoll,9> rolls{};
    if(s->rng_mode==2) {
        FILE* f=fopen(LBX_REQUEST,"rb");char line[256];int dex,attempt,best;unsigned ec,pid,ivs;char profile[65];
        if(!f)return false;
        bool ok=!!fgets(line,sizeof(line),f) && !strncmp(line,"LBXR4\t",6);
        while(ok && fgets(line,sizeof(line),f)) {
            if(sscanf(line,"%d\t%x\t%x\t%x\t%d\t%d\t%64s",&dex,&ec,&pid,&ivs,&attempt,&best,profile)!=7 ||
               dex<1 || dex>9 || strcmp(profile,device_profiles[dex-1].hash)) {ok=false;break;}
            rolls[dex-1]={ec,pid,ivs};
        }
        fclose(f);if(!ok){snprintf(error,size,"Local request/profile mismatch.");return false;}
    }
    FILE* f=fopen(LBX_ROOT "/templates/templates.tsv","rb");
    if(!f){snprintf(error,size,"Install reviewed templates pack first.");return false;}
    char line[256],name[96],digest[65],actual[65];int count=0,dex=0;bool seen[807]{};
    bool ok=!!fgets(line,sizeof(line),f) && sscanf(line,"LBXT1\t%d",&count)==1 && count>0 && count<=807;
    std::vector<Candidate> candidates;int index=0;
    for(int i=0;ok && i<count;i++) {
        ok=!!fgets(line,sizeof(line),f) && sscanf(line,"%d\t%95[^\t]\t%64s",&dex,name,digest)==3 &&
           dex>=1 && dex<=807 && !seen[dex-1] && safe_name(name) && strlen(digest)==64;
        if(!ok)break;seen[dex-1]=true;
        if(!s->selected[dex-1])continue;
        char path[256];Bytes bytes;snprintf(path,sizeof(path),LBX_ROOT "/templates/%s",name);
        ok=read_file(path,bytes,260) && (bytes.size()==232 || bytes.size()==260);
        if(!ok)break;hash(bytes.data(),bytes.size(),actual);ok=!strcmp(actual,digest);if(!ok)break;
        auto pk=pksm::PKX::getPKM<pksm::Generation::SEVEN>(bytes.data(),bytes.size());
        ok=pk && valid_pk(*pk) && u16(pk->species())==dex;
        if(!ok)break;
        if(s->rng_mode==2) {
            // First supported recipes are the reviewed Kanto starters, not arbitrary PK7 edits.
            ok=dex<=9 && (pk->version()==pksm::GameVersion::RD || pk->version()==pksm::GameVersion::GN || pk->version()==pksm::GameVersion::YW);
            if(!ok)break;
            if(s->level && s->level<pk->level()) {
                snprintf(error,size,"#%03d: local level must be >= reviewed level %d.",dex,pk->level());fclose(f);return false;
            }
            bool shiny=s->shiny?s->shiny==1:pk->shiny();
            pk->encryptionConstant(rolls[dex-1].ec);pk->PID(rolls[dex-1].pid);
            uint32_t pid=pk->PID();
            if(shiny)pk->PID((pid&0xffff)|((uint32_t)((u16)pid^pk->TID()^pk->SID())<<16));
            else if(pk->shiny())pk->PID(pid^0x10000000u);
            for(unsigned j=0;j<6;j++)pk->iv((pksm::Stat)j,(rolls[dex-1].ivs>>(5*j))&31);
            if(s->level)pk->level((u8)s->level);
            if(s->nature)pk->nature(pksm::Nature((u8)(s->nature-1)));
            pk->refreshChecksum();
        }
        candidates.push_back({std::move(pk),s->start_box+index/30,index%30});index++;
    }
    fclose(f);
    if(!ok || index!=lbx_selected_count(s)) {
        snprintf(error,size,"Missing/damaged templates or unsupported local recipe.");return false;
    }
    return stage(candidates,io,error,size);
}
static bool commit_save(char* error,size_t size) {
    if(!pending){snprintf(error,size,"Prepare and review before saving.");return false;}
    auto p=std::move(pending);Bytes live(LBX_MOON_SAVE_SIZE);
    if(!p->io.read(live.data(),live.size(),error,size,p->io.context) || live!=p->before) {
        snprintf(error,size,"Moon save changed/removed since preview. Prepare again.");return false;
    }
    snprintf(error,size,"Cannot create verified backup. Game unchanged.");
    if(!backup(p->before,error,size))return false;
    bool wrote=p->io.write(p->after.data(),p->after.size(),error,size,p->io.context);
    bool read=p->io.read(live.data(),live.size(),error,size,p->io.context);
    if(wrote && read && live==p->after) {
        snprintf(error,size,"Saved %d Pokemon to Moon; full readback passed. Open Bank.",p->count);return true;
    }
    // A partial write or uncertain commit is never reported as success.
    char restore_error[160];
    bool restored=p->io.write(p->before.data(),p->before.size(),restore_error,sizeof(restore_error),p->io.context) &&
                  p->io.read(live.data(),live.size(),restore_error,sizeof(restore_error),p->io.context) && live==p->before;
    snprintf(error,size,restored?"Save failed. Original restored and verified. Do not use Bank.":
             "Save failed; restore not verified. Use Moon backup in /3ds/LegacyBox/backups.");return false;
}

extern "C" bool lbx_moon_prepare_batch(const LbxSaveIO* io,char* error,size_t size) {
    try {return prepare_batch(io,error,size);} catch(const std::exception& e) {
        pending.reset();snprintf(error,size,"Prepare failed: %.100s. No game writes.",e.what());return false;
    }
}
extern "C" bool lbx_moon_prepare(const LbxFrontend* s,const LbxSaveIO* io,LbxSearchProgress progress,
                                 void* context,char* error,size_t size) {
    try {return prepare_selected(s,io,progress,context,error,size);} catch(const std::exception& e) {
        pending.reset();snprintf(error,size,"Prepare failed: %.100s. No game writes.",e.what());return false;
    }
}
extern "C" bool lbx_moon_commit(char* error,size_t size) {
    try {return commit_save(error,size);} catch(const std::exception& e) {
        pending.reset();snprintf(error,size,"Save interrupted: %.80s. Check Moon backup before Bank.",e.what());return false;
    }
}
