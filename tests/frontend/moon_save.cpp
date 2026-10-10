#include "moon_save.h"
extern "C" {
#include "legacybox.h"
#include "sha256.h"
}
#include "sav/SavSUMO.hpp"
#include "pkx/PK7.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
using Bytes=std::vector<uint8_t>;
namespace fs=std::filesystem;
static Bytes live;static int writes=0,fail_write=0;static char error[256];
static bool read(uint8_t* p,size_t n,char*,size_t,void*) {if(n!=live.size())return false;memcpy(p,live.data(),n);return true;}
static bool write(const uint8_t* p,size_t n,char*,size_t,void*) {
    ++writes;
    if(writes==fail_write) {memcpy(live.data(),p,n/2);return false;}
    live.assign(p,p+n);return true;
}
static LbxSaveIO io={read,write,nullptr,"Test Moon"};
static std::shared_ptr<uint8_t[]> copy_data(const Bytes& b) {
    auto p=std::shared_ptr<uint8_t[]>(new uint8_t[b.size()]);memcpy(p.get(),b.data(),b.size());return p;
}
static Bytes blank() {
    auto p=std::shared_ptr<uint8_t[]>(new uint8_t[LBX_MOON_SAVE_SIZE]{});
    // The final block excludes its signature when computing CRC; real saves carry these block IDs.
    for(int i=0;i<37;i++){p[0x6BC18+i*8]=i;p[0x6BC19+i*8]=0;}
    pksm::SavSUMO save(p);save.version(pksm::GameVersion::MN);save.language(pksm::Language::ENG);
    save.otName("Kento");save.TID(123);save.SID(456);save.finishEditing();assert(save.checksumsValid());
    return Bytes(p.get(),p.get()+LBX_MOON_SAVE_SIZE);
}
static void file(const fs::path& p,const Bytes& b) {std::ofstream f(p,std::ios::binary);f.write((const char*)b.data(),b.size());assert(f.good());}
static Bytes load(const fs::path& p) {std::ifstream f(p,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(f),{});}
static std::string sha(const Bytes& b) {
    LbxSha256 h;uint8_t d[32];char out[65];lbx_sha256_init(&h);lbx_sha256_update(&h,b.data(),b.size());lbx_sha256_final(&h,d);lbx_sha256_hex(d,out);return out;
}
static Bytes pokemon() {
    Bytes b(232);auto pk=pksm::PKX::getPKM<pksm::Generation::SEVEN>(b.data(),b.size());
    pk->species(pksm::Species::Bulbasaur);pk->version(pksm::GameVersion::RD);pk->language(pksm::Language::ENG);
    pk->level(5);pk->refreshChecksum();return Bytes(pk->rawData().begin(),pk->rawData().end());
}
static void batch(const Bytes& pk,int box) {
    fs::create_directories(LBX_INBOX);file(fs::path(LBX_INBOX)/"001.pk7",pk);
    std::string material="001.pk7:"+sha(pk)+":"+std::to_string(box)+":0";
    Bytes bytes(material.begin(),material.end());
    std::ofstream f(LBX_MANIFEST);f<<"LBX1\t"<<sha(bytes)<<"\t1\t"<<box<<"\n001.pk7\t"<<sha(pk)<<"\t"<<box<<"\t0\n";
}
static void tests() {
    fs::remove_all(LBX_ROOT);fs::create_directories(LBX_ROOT);auto original=blank(),pk=pokemon();batch(pk,1);
    live=original;writes=0;
    assert(lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(writes==0 && live==original && lbx_moon_pending());
    lbx_moon_discard();assert(live==original && !lbx_moon_pending());
    assert(lbx_moon_prepare_batch(&io,error,sizeof(error)));live[0]^=1;
    assert(!lbx_moon_commit(error,sizeof(error)));assert(writes==0);
    live=original;batch(pk,7);assert(!lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(writes==0);
    batch(pk,1);live[0]^=1;assert(!lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(writes==0);
    live=original;assert(lbx_moon_prepare_batch(&io,error,sizeof(error)));fail_write=1;
    assert(!lbx_moon_commit(error,sizeof(error)));assert(live==original && writes==2);
    fail_write=0;writes=0;assert(lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(lbx_moon_commit(error,sizeof(error)));assert(writes==1);
    auto changed=copy_data(live);pksm::SavSUMO save(changed);assert(save.checksumsValid());auto got=save.pkm(1,0);
    assert(got->species()==pksm::Species::Bulbasaur && !got->isEncrypted());
    // Existing originals outside the destination are byte-preserved, including protected Box 8.
    assert(!memcmp(live.data()+0x4E00+7*30*232,original.data()+0x4E00+7*30*232,30*232));
    assert(!lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(writes==1);
    live=original;auto backup=fs::path(LBX_ROOT)/"backups"/("moon-"+sha(original)+".main");file(backup,Bytes{1,2,3});
    assert(lbx_moon_prepare_batch(&io,error,sizeof(error)));assert(!lbx_moon_commit(error,sizeof(error)));assert(writes==1 && live==original);
    fs::remove_all(LBX_ROOT);fs::create_directories(LBX_ROOT);
    LbxFrontend state{};lbx_frontend_init(&state);state.selected[1]=true;state.rng_mode=2;
    assert(!lbx_moon_prepare(&state,&io,nullptr,nullptr,error,sizeof(error)));assert(writes==1);
    puts("Moon tests passed: preview, cancellation, changed save, Box 8, CRC, rollback, readback, occupied slots, backup corruption, missing templates.");
}
int main(int argc,char** argv) {
    tests();
    if(argc<3)return 0;
    // Exercise actual PKHeX-passed templates. Emit saved/read-back PK7 for independent PKHeX validation.
    fs::copy(argv[1],fs::path(LBX_ROOT)/"templates",fs::copy_options::recursive);
    fs::create_directories(argv[2]);
    for(int variant=0;variant<4;variant++) {
        live=blank();writes=0;LbxFrontend s{};lbx_frontend_init(&s);for(int i=0;i<9;i++)s.selected[i]=true;
        s.start_box=1;s.rng_mode=variant?2:0;s.rng_budget=0;s.rng_seed=0x12345678u+variant;
        if(variant==2){s.level=100;s.nature=16;s.shiny=1;}
        if(variant==3){s.level=100;s.nature=4;s.shiny=2;s.rng_budget=2;}
        if(!lbx_moon_prepare(&s,&io,nullptr,nullptr,error,sizeof(error))) {fprintf(stderr,"prepare: %s\n",error);return 1;}
        assert(lbx_moon_commit(error,sizeof(error)));assert(writes==1);
        auto p=copy_data(live);pksm::SavSUMO save(p);assert(save.checksumsValid());
        for(int i=0;i<9;i++) {
            auto pk=save.pkm(1,(u8)i);assert(u16(pk->species())==i+1 && static_cast<pksm::PK7&>(*pk).htName()=="Kento");
            auto path=fs::path(argv[2])/(std::to_string(variant)+"-"+std::to_string(i+1)+".pk7");
            file(path,Bytes(pk->rawData().begin(),pk->rawData().end()));
        }
        file(fs::path(argv[2])/("moon-"+std::to_string(variant)+".main"),live);
    }
    puts("Saved and read back 36 native Pokemon across preserve/device/level/nature/shiny variants.");
}
