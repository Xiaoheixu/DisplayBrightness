#include "ImageGate.hpp"
#include <cassert>
#include <cstdio>
int main(){
 const unsigned char expected[16]={1};
 struct Image{mach_header_64 h;uuid_command u;} image={};
 image.h.magic=MH_MAGIC_64;image.h.ncmds=1;image.h.sizeofcmds=sizeof(image.u);
 image.u.cmd=LC_UUID;image.u.cmdsize=sizeof(image.u);image.u.uuid[0]=1;
 assert(dbImageUUIDMatches(&image,sizeof(image),expected));
 assert(!dbImageUUIDMatches(nullptr,sizeof(image),expected));
 assert(!dbImageUUIDMatches(&image,sizeof(image)-1,expected));
 image.u.uuid[0]=2;assert(!dbImageUUIDMatches(&image,sizeof(image),expected));image.u.uuid[0]=1;
 image.u.cmdsize=0;assert(!dbImageUUIDMatches(&image,sizeof(image),expected));image.u.cmdsize=sizeof(image.u);
 image.h.ncmds=2;assert(!dbImageUUIDMatches(&image,sizeof(image),expected));image.h.ncmds=1;
 image.h.magic=0;assert(!dbImageUUIDMatches(&image,sizeof(image),expected));
 image.h.magic=MH_MAGIC_64;
 auto match=[&](unsigned major,unsigned minor,bool enabled,const unsigned char *uuid){
  memcpy(image.u.uuid,uuid,16);
  return dbMatchGraphicsProfile(major,minor,enabled,&image,sizeof(image));
 };
 const unsigned char sonoma[]={0x79,0xf2,0xf7,0xe4,0xe4,0x51,0x35,0xe1,0xbf,0x22,0x1e,0xff,0xa4,0x19,0x28,0x88};
 const unsigned char sequoia[]={0xca,0x8b,0x49,0xbc,0x36,0x55,0x31,0x2c,0x91,0xaa,0xfb,0x84,0x82,0x51,0xc7,0x8e};
 const unsigned char ventura[]={0x71,0x8e,0x01,0xcf,0x8b,0x05,0x30,0x42,0x88,0xf4,0xde,0x34,0x41,0x39,0x5d,0x00};
 const auto &tahoe=dbGraphicsProfiles[0];
 assert(match(25,6,false,tahoe.uuid)==&tahoe);
 assert(match(25,6,true,tahoe.uuid)==&tahoe);
 assert(!match(23,6,false,sonoma) && !match(24,6,false,sequoia));
 assert(match(23,6,true,sonoma) && match(24,6,true,sequoia));
 assert(!match(23,6,true,sequoia) && !match(24,6,true,sonoma));
 assert(!match(24,5,true,sequoia) && !match(25,6,true,sequoia));
 assert(!match(22,6,false,ventura));
 assert(match(22,6,true,ventura));
 assert(!match(22,5,true,ventura) && !match(23,6,true,ventura));
 assert(!match(22,6,true,sonoma) && !match(26,6,true,tahoe.uuid));
 assert(!match(24,6,true,expected));
 puts("image gate: matching UUID accepted; null/truncation/mismatch/invalid commands rejected");
}
