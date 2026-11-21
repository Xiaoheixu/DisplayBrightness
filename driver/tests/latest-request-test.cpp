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
 assert(q.hasNewTarget());
 assert(q.complete(false,true) && q.take()==80);
 assert(q.automaticFailureRetries==0);
 for(unsigned retry=0;retry<2;retry++){
  assert(q.complete(false,true));
  assert(q.take()==80);
 }
 // Duplicate callbacks must not rearm a target whose retries are exhausted.
 assert(q.submit(80,start) && !start && !q.hasNewTarget());
 assert(!q.complete(false,true) && !q.pending && !q.busy);
 assert(q.submit(90,start) && start && q.take()==90);
 assert(q.complete(false,true) && q.take()==90);
 assert(!q.complete(true,true));
 assert(q.submit(25,start) && start && q.take()==25);
 assert(!q.complete(false,true,0) && q.failed && !q.pending && !q.busy);
 // Coalescing a different target after the fresh read starts its own budget.
 assert(q.submit(45,start) && start && q.take()==45);
 assert(q.complete(false,true) && q.take()==45);
 assert(q.submit(55,start) && !start);
 unsigned target=45;uint64_t seq=1;
 assert(q.refreshAfterRead(2,target,seq) && target==55 && seq==2 && q.automaticFailureRetries==0);
 assert(q.complete(false,true) && q.take()==55);
 assert(q.submit(55,start) && !start);
 assert(q.refreshAfterRead(3,target,seq) && target==55 && seq==3 && q.automaticFailureRetries==1);
 assert(q.complete(false,true,0)==false);
 // A deferral after bootstrap publication must never turn the startup
 // placeholder (zero) into an unsolicited brightness write.
 dbhw::LatestRequest boot;
 assert(boot.submit(0,start)&&start);assert(boot.take()==0);
 assert(!boot.defer(true));assert(!boot.busy&&!boot.pending&&!boot.failed);
 // A real request arriving just after publication still has to resume.
 assert(boot.submit(75,start)&&start);assert(boot.take()==75);
 assert(boot.submit(90,start)&&!start);
 assert(boot.defer(true));assert(boot.take()==90);assert(!boot.complete(true));
 // Ordinary interrupted work keeps its consumed target, without a failure.
 assert(boot.submit(40,start)&&start);assert(boot.take()==40);
 assert(boot.defer());assert(boot.busy&&boot.pending&&!boot.failed);
 assert(boot.take()==40);assert(!boot.complete(true));
 boot.stop();assert(!boot.defer());assert(!boot.pending&&!boot.busy);
 // Only a distinct target can bring forward a pending failure retry.
 dbhw::LatestRequest cooling;
 unsigned cancellations=0;
 auto cancel=[&](){++cancellations;return true;};
 assert(cooling.submit(30,start) && start && cooling.take()==30);
 assert(cooling.complete(false,true));
 cooling.waitingForRetry=true;
 assert(cooling.submit(30,start) && !start && !cooling.advanceRetry(cancel));
 assert(cancellations==0 && cooling.waitingForRetry);
 assert(cooling.submit(70,start) && !start && cooling.advanceRetry(cancel));
 assert(cancellations==1 && !cooling.waitingForRetry);
 assert(cooling.submit(80,start) && !start && !cooling.advanceRetry(cancel));
 assert(cooling.take()==80 && cooling.automaticFailureRetries==0);
 assert(cooling.complete(false,true));
 cooling.waitingForRetry=true;
 // The call can already be running. It will consume the latest target.
 assert(cooling.submit(90,start) && !start);
 assert(!cooling.advanceRetry([&](){++cancellations;return false;}));
 assert(cancellations==2 && cooling.pending && !cooling.waitingForRetry);
 assert(cooling.take()==90);
 cooling.waitingForRetry=true;
 cooling.stop();
 assert(!cooling.waitingForRetry && !cooling.advanceRetry(cancel));
 // A display-update deferral must clear the failure-timer state, so new
 // input cannot mistake the guard timer for a failure cooldown.
 dbhw::LatestRequest guarded;
 assert(guarded.submit(10,start)&&start);guarded.take();
 assert(guarded.complete(false,true));guarded.waitingForRetry=true;
 assert(guarded.defer());assert(!guarded.waitingForRetry);
 assert(guarded.submit(20,start)&&!start);
 assert(!guarded.advanceRetry(cancel));
 assert(guarded.automaticFailureRetries==1);
 puts("request policy: unlimited successful cycles, bounded failure retries, new input preserved");
}
