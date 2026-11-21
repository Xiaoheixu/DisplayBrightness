#pragma once
#include <stdint.h>
namespace dbhw {
// doUpdate is only a wake/topology hint. Caller serializes this state.
struct DisplayUpdateGuard {
 static constexpr uint64_t quietNs=2000000000ULL;
 uint64_t generation=0,untilNs=0;
 unsigned depth=0;
 void begin(){++depth;++generation;}
 void end(uint64_t now){
  if(depth)--depth;
  const uint64_t deadline=now>UINT64_MAX-quietNs?UINT64_MAX:now+quietNs;
  if(deadline>untilNs)untilNs=deadline;
 }
 bool ready(uint64_t now)const{return !depth&&now>=untilNs;}
 bool permits(uint64_t now,uint64_t captured)const{return ready(now)&&captured==generation;}
};
}
