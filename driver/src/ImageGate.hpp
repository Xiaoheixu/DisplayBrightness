#pragma once
#include <mach-o/loader.h>
#include <stddef.h>
#include <string.h>
inline bool dbImageUUIDMatches(const void *image,size_t length,const unsigned char expected[16]){
    if(!image || length<sizeof(mach_header_64))return false;
    const auto *h=static_cast<const mach_header_64 *>(image);
    if(h->magic!=MH_MAGIC_64 || h->sizeofcmds>length-sizeof(*h))return false;
    const unsigned char *cursor=reinterpret_cast<const unsigned char *>(h+1);
    const unsigned char *end=cursor+h->sizeofcmds;
    bool matched=false;
    for(unsigned i=0;i<h->ncmds;i++){
        if(static_cast<size_t>(end-cursor)<sizeof(load_command))return false;
        const auto *cmd=reinterpret_cast<const load_command *>(cursor);
        if(cmd->cmdsize<sizeof(*cmd) || cmd->cmdsize>static_cast<size_t>(end-cursor))return false;
        if(cmd->cmd==LC_UUID){
            if(matched || cmd->cmdsize<sizeof(uuid_command))return false;
            const auto *uuid=reinterpret_cast<const uuid_command *>(cursor);
            if(memcmp(uuid->uuid,expected,16))return false;
            matched=true;
        }
        cursor+=cmd->cmdsize;
    }
    return matched && cursor==end;
}

// Each image is tied to the kernel ABI against which it was examined.
struct DBGraphicsProfile {
    unsigned major;
    unsigned minor;
    bool experimental;
    unsigned char uuid[16];
    const char *referenceBuild;
};
constexpr DBGraphicsProfile dbGraphicsProfiles[] = {
    {25, 6, false, {0x74,0xaa,0x64,0x83,0x8a,0xb4,0x3a,0x94,0x81,0x43,0x5e,0x30,0x6c,0x16,0xd7,0xda}, "25G332"},
    {24, 6, true, {0xca,0x8b,0x49,0xbc,0x36,0x55,0x31,0x2c,0x91,0xaa,0xfb,0x84,0x82,0x51,0xc7,0x8e}, "24H232"},
    {23, 6, true, {0x79,0xf2,0xf7,0xe4,0xe4,0x51,0x35,0xe1,0xbf,0x22,0x1e,0xff,0xa4,0x19,0x28,0x88}, "23J631"},
    {22, 6, true, {0x71,0x8e,0x01,0xcf,0x8b,0x05,0x30,0x42,0x88,0xf4,0xde,0x34,0x41,0x39,0x5d,0x00}, "22H625"},
};
inline const DBGraphicsProfile *dbMatchGraphicsProfile(unsigned major, unsigned minor,
    bool legacyAllowed, const void *image, size_t length) {
    for (const auto &profile : dbGraphicsProfiles) {
        if (profile.major == major && profile.minor == minor &&
            (!profile.experimental || legacyAllowed) &&
            dbImageUUIDMatches(image, length, profile.uuid)) {
            return &profile;
        }
    }
    return nullptr;
}
