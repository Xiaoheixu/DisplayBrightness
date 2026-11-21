#include "RetirePolicy.hpp"
#include "LatestRequest.hpp"
#include "BootstrapPending.hpp"
#include <cassert>
#include <cstdio>
int main(){
 // dbslots=1: the sole slot is disconnected but still owns a delayed job.
 dbhw::LatestRequest old;bool schedule=false,retiring=false;
 assert(old.submit(50,schedule)&&schedule);
 assert(!dbhw::canRetire(true,true,old.busy,retiring,false));
 dbstate::BootstrapPending<int,1> tickets;int display=1,frame=2;
 if(dbhw::retirementExpected(true,true,retiring))
  assert(tickets.offer(&display,&frame)==dbstate::Offer::Inserted);
 assert(tickets.peek(0).display==&display);
 // Its callback now observes disconnect. It must trigger retirement once,
 // so cleanup can wake this exact retained ticket without a second update.
 old.stop();
 assert(dbhw::markInactiveForRetirement(true,retiring));
 assert(retiring&&!old.busy);
 assert(!dbhw::markInactiveForRetirement(true,retiring));
 auto ticket=tickets.take(0);
 assert(ticket.display==&display&&ticket.frame==&frame);
 assert(!tickets.peek(0).display);
 // Active full slots don't create permanent admission waiters/polling.
 assert(!dbhw::retirementExpected(true,false,false));
 assert(!dbhw::retirementExpected(false,true,false));
 assert(dbhw::retirementExpected(true,false,true));
 retiring=false;assert(!dbhw::markInactiveForRetirement(false,retiring));
 assert(!retiring);
 // Pending admission is bounded and deduplicates updates.
 assert(tickets.offer(&display,&frame)==dbstate::Offer::Inserted);
 assert(tickets.offer(&display,&frame)==dbstate::Offer::Existing);
 int another=3;assert(tickets.offer(&another,&frame)==dbstate::Offer::Full);
 puts("retirement admission: inactive delayed slot preserves ticket, single cleanup trigger, active-full bound pass");
}
