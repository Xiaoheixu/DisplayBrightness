#include "DisplayUpdateGuard.hpp"
#include "LatestRequest.hpp"
#include "ResponsiveRead.hpp"
#include "BootstrapWindow.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using dbhw::DisplayUpdateGuard;
 DisplayUpdateGuard g;
 assert(g.ready(0));
 g.begin();auto old=g.generation;
 assert(!g.ready(10000000000ULL));
 g.begin();g.end(100);assert(!g.ready(3000000000ULL));
 g.end(1000000000ULL);
 assert(!g.ready(2999999999ULL));assert(g.ready(3000000000ULL));
 g.begin();g.end(2500000000ULL);
 assert(!g.ready(3000000000ULL));assert(g.ready(4500000000ULL));
 assert(!g.permits(4500000000ULL,old));
 assert(g.permits(4500000000ULL,g.generation));
 // A display update between read and write must invalidate the read even
 // when the worker is delayed until after the quiet period has elapsed.
 dbhw::LatestRequest q;bool schedule=false;
 assert(q.submit(20,schedule)&&schedule);
 unsigned desired=q.take();auto cycle=g.generation;
 unsigned reads=1,writes=0;
 g.begin();g.end(5000000000ULL);
 assert(q.submit(80,schedule)&&!schedule);
 if(g.permits(7000000000ULL,cycle))++writes;
 assert(writes==0);
 // Deferral retains busy/latest state and must not mark a transport failure.
 assert(q.defer());assert(q.busy&&q.pending&&!q.failed);
 desired=q.take();cycle=g.generation;++reads;
 if(g.permits(7000000000ULL,cycle)&&desired==80)++writes;
 assert(reads==2&&writes==1);assert(!q.complete(true));
 // The production retry helper detects an update at the next transport
 // checkpoint. It may finish its retry wait, but must not access the bus.
 DisplayUpdateGuard retryGuard;auto retryCycle=retryGuard.generation;
 bool deferred=false;unsigned calls=0,pauses=0;
 auto result=dbhw::responsiveRead([&](){
  if(!retryGuard.permits(0,retryCycle)){deferred=true;return false;}
  ++calls;return false;
 },[&](){
  if(++pauses==1){retryGuard.begin();retryGuard.end(0);}
 },[&](){return !deferred;},[](){return false;});
 assert(result==dbhw::ReadResult::Stopped&&calls==1&&pauses==25);
 // No startup visit is taken while deferred, even after a long bus wait.
 dbstate::BootstrapWindow window;
 retryGuard.begin();
 if(retryGuard.ready(90000000000ULL))window.begin(90000000000ULL);
 assert(!window.begun&&window.rounds==0);
 retryGuard.end(90000000000ULL);
 assert(retryGuard.ready(92000000000ULL));
 assert(window.begin(92000000000ULL)&&window.rounds==1);
 assert(window.within(151999999999ULL));assert(!window.within(152000000000ULL));
 // Initial admission gets a quiet interval even without a prior update.
 DisplayUpdateGuard initial;initial.begin();initial.end(0);
 assert(!initial.ready(1999999999ULL));assert(initial.ready(2000000000ULL));
 DisplayUpdateGuard saturated;saturated.begin();saturated.end(UINT64_MAX-1);
 assert(!saturated.ready(UINT64_MAX-1));assert(saturated.ready(UINT64_MAX));
 // Independent endpoints are unaffected.
 DisplayUpdateGuard other;g.begin();assert(other.ready(0));
 puts("display-update guard: nested updates, quiet deadline, stale cycles and latest target pass");
}
