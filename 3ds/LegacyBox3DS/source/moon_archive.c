// Moon-only libctru save archive adapter. Archive layout follows PKSM's TitleLoader.
#include <3ds.h>
#include "moon_save.h"
#include <stdio.h>
#define MOON_ID 0x0004000000175E00ULL
static FS_MediaType target_media;
static bool target_valid;
static bool moon_on(FS_MediaType media) {
    u32 count=0,read=0;u64 ids[8];
    if(R_FAILED(AM_GetTitleCount(media,&count)) || !count || count>8)return false;
    if(R_FAILED(AM_GetTitleList(&read,media,count,ids)))return false;
    for(u32 i=0;i<read;i++)if(ids[i]==MOON_ID)return true;
    return false;
}
static bool card_unchanged(void) {
    if(R_FAILED(amInit()))return false;
    bool ok=moon_on(MEDIATYPE_GAME_CARD);amExit();return ok;
}
static bool archive_transfer(uint8_t* data,size_t size,bool write,char* error,size_t error_size) {
    if(!target_valid || size!=LBX_MOON_SAVE_SIZE ||
       (target_media==MEDIATYPE_GAME_CARD && !card_unchanged())) {
        snprintf(error,error_size,"Moon cartridge removed/changed. No save access.");return false;
    }
    u32 path_data[3]={target_media,(u32)MOON_ID,(u32)(MOON_ID>>32)};
    FS_Path path={PATH_BINARY,sizeof(path_data),path_data};FS_Archive archive;Handle file=0;
    Result r=FSUSER_OpenArchive(&archive,ARCHIVE_USER_SAVEDATA,path);
    if(R_FAILED(r)){snprintf(error,error_size,"Moon archive access failed (%08lX). Check CFW permissions.",(unsigned long)r);return false;}
    r=FSUSER_OpenFile(&file,archive,fsMakePath(PATH_ASCII,"/main"),write?FS_OPEN_WRITE:FS_OPEN_READ,0);
    u64 length=0;u32 transferred=0;
    if(R_SUCCEEDED(r))r=FSFILE_GetSize(file,&length);
    if(R_SUCCEEDED(r) && length!=size)r=(Result)-1;
    if(R_SUCCEEDED(r)) {
        if(write)r=FSFILE_Write(file,&transferred,0,data,(u32)size,FS_WRITE_FLUSH);
        else r=FSFILE_Read(file,&transferred,0,data,(u32)size);
    }
    bool ok=R_SUCCEEDED(r) && transferred==size;
    if(file && R_FAILED(FSFILE_Close(file)))ok=false;
    if(write && ok) {
        r=FSUSER_ControlArchive(archive,ARCHIVE_ACTION_COMMIT_SAVE_DATA,NULL,0,NULL,0);
        ok=R_SUCCEEDED(r);
    }
    if(R_FAILED(FSUSER_CloseArchive(archive)))ok=false;
    if(write && ok && target_media==MEDIATYPE_SD) {
        u64 secure=((u64)SECUREVALUE_SLOT_SD<<32)|((u32)MOON_ID&0xFFFFFF00u);u8 out=0;
        r=FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE,&secure,sizeof(secure),&out,sizeof(out));
        ok=R_SUCCEEDED(r);
    }
    if(!ok)snprintf(error,error_size,"Moon %s/commit failed (%08lX).",write?"write":"read",(unsigned long)r);
    return ok;
}
static bool read_save(uint8_t* data,size_t size,char* error,size_t error_size,void* context) {
    (void)context;return archive_transfer(data,size,false,error,error_size);
}
static bool write_save(const uint8_t* data,size_t size,char* error,size_t error_size,void* context) {
    (void)context;return archive_transfer((uint8_t*)data,size,true,error,error_size);
}
bool lbx_moon_archive(LbxSaveIO* io,char* error,size_t error_size) {
    target_valid=false;
    Result r=amInit();
    if(R_FAILED(r)){snprintf(error,error_size,"Title access failed (%08lX).",(unsigned long)r);return false;}
    if(moon_on(MEDIATYPE_GAME_CARD))target_media=MEDIATYPE_GAME_CARD;
    else {
        // Installed libraries may contain more than eight titles: query Moon directly via archive.
        target_media=MEDIATYPE_SD;
    }
    amExit();target_valid=true;
    *io=(LbxSaveIO){read_save,write_save,NULL,target_media==MEDIATYPE_GAME_CARD?"Moon cartridge":"Moon digital"};
    snprintf(error,error_size,"Moon %s selected.",target_media==MEDIATYPE_GAME_CARD?"cartridge":"digital");return true;
}
