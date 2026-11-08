#pragma once
#include <stdint.h>
namespace dbstate {
struct BootstrapWindow {
 static constexpr unsigned intervalMs=1000,maximumRounds=60;
 static constexpr uint64_t durationNs=60000000000ULL;
 uint64_t firstNs=0,budgetNs=durationNs;unsigned rounds=0,cadenceMs=intervalMs;bool begun=false;
 void configure(unsigned seconds,unsigned interval){budgetNs=static_cast<uint64_t>(seconds)*1000000000ULL;cadenceMs=interval;}
 bool within(uint64_t now)const{return begun&&now>=firstNs&&now-firstNs<budgetNs;}
 void observe(uint64_t now){if(!begun){begun=true;firstNs=now;}}
 bool begin(uint64_t now){observe(now);if(!within(now)||rounds>=maximumRounds)return false;++rounds;return true;}
 bool maySchedule(uint64_t now)const{return within(now)&&rounds<maximumRounds&&budgetNs-(now-firstNs)>static_cast<uint64_t>(cadenceMs)*1000000ULL;}
};
}
