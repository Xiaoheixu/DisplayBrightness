#include "BootstrapPending.hpp"
#include <cassert>
#include <cstdio>
int main(){dbstate::BootstrapPending<int,2> p;int a=1,b=2,c=3,fa=4,fb=5,fc=6;
 assert(p.offer(&a,&fa)==dbstate::Offer::Inserted);
 assert(p.offer(&a,&fa)==dbstate::Offer::Existing);
 assert(p.offer(&b,&fb)==dbstate::Offer::Inserted);
 assert(p.offer(&c,&fc)==dbstate::Offer::Full);
 auto t=p.take(0);assert(t.display==&a&&t.frame==&fa);
 assert(!p.take(0).display);
 assert(p.offer(&c,&fc)==dbstate::Offer::Inserted);
 auto u=p.take(1);assert(u.display==&b&&u.frame==&fb);
 assert(!p.take(2).display);
 dbstate::BootstrapPending<int,2> recovery;
 assert(recovery.offer(&a,&fa)==dbstate::Offer::Inserted);
 assert(recovery.offer(&a,&fa,true)==dbstate::Offer::Existing);
 assert(recovery.offer(&a,&fa,false)==dbstate::Offer::Existing);
 assert(recovery.peek(0).recovery && recovery.recoveryPending(&a,&fa));
 assert(!recovery.recoveryPending(&a,&fb));
 assert(!recovery.takeMatching(0,&a,&fb).display && recovery.recoveryPending(&a,&fa));
 assert(recovery.offer(&b,&fb,true)==dbstate::Offer::Inserted);
 assert(recovery.offer(&c,&fc,true)==dbstate::Offer::Full);
 assert(!recovery.takePair(&c,&fc).display && recovery.recoveryPending(&a,&fa));
 assert(recovery.takePair(&b,&fb).recovery);
 auto modeTicket=recovery.takeMatching(0,&a,&fa);
 assert(!recovery.recoveryPending(&a,&fa));
 assert(modeTicket.display==&a && modeTicket.frame==&fa && modeTicket.recovery);
 puts("bootstrap admission: dedupe, bounded capacity, no overwrite, exact ownership transfer and reuse pass");}
