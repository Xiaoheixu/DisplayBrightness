#include "LatestRequest.hpp"
#include <cassert>
#include <cstdio>
int main(){
 dbhw::LatestRequest q;bool start=false;
 assert(q.submit(10,start)&&start);assert(q.take()==10);
 assert(q.submit(30,start)&&!start);
 assert(q.complete(false)); // failure must schedule automatic recovery
 assert(q.failed&&q.busy);assert(q.submit(80,start)&&!start);
 assert(q.take()==80);assert(!q.complete(true));assert(!q.failed);
 for(unsigned i=0;i<1000;i++){
  unsigned v=i%101;assert(q.submit(v,start)&&start);assert(q.take()==v);assert(!q.complete(true));
 }
 assert(!q.submit(101,start));
 assert(q.submit(70,start));assert(q.take()==70);
 for(unsigned i=0;i<10;i++){assert(q.complete(false));assert(q.take()==70);}
 assert(!q.complete(true));
 // Initialized hardware can fail persistently, for example after a mode switch.
 assert(q.submit(60,start) && start && q.take()==60);
 for(unsigned retry=0;retry<2;retry++){
  assert(q.complete(false,true));
  assert(q.take()==60);
 }
 assert(!q.complete(false,true) && !q.busy && !q.pending && q.failed);
 // A fresh user request can retry. Exhaustion cannot discard newer input.
 assert(q.submit(75,start) && start && q.take()==75);
 for(unsigned retry=0;retry<2;retry++){
  assert(q.complete(false,true));
  assert(q.take()==75);
 }
 assert(q.submit(80,start) && !start);
 assert(q.complete(false,true) && q.take()==80);
 assert(!q.complete(false,true));
 assert(q.submit(90,start) && start && q.take()==90);
 assert(q.complete(false,true) && q.take()==90);
 assert(!q.complete(true,true));
 assert(q.submit(25,start) && start && q.take()==25);
 assert(!q.complete(false,true,0) && q.failed && !q.pending && !q.busy);
 puts("request policy: unlimited successful cycles, bounded failure retries, new input preserved");
}
