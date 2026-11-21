#include "EndpointRefresh.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using dbhw::Refresh;using dbhw::endpointRefresh;
 assert(endpointRefresh(true,false,true,true,true,false,true)==Refresh::Keep);
 assert(dbhw::interfaceUsable(1,true,false,true));
 assert(!dbhw::interfaceUsable(1,true,true,true));
 assert(!dbhw::interfaceUsable(2,true,false,true));
 assert(!dbhw::interfaceUsable(0,false,false,false));
 // USB-C reconfiguration can leave the old service attached during teardown.
 int oldBus=0, newBus=0;
 dbhw::ActiveInterfaces<int> buses;
 buses.observe(&oldBus,true);
 buses.observe(&newBus,false);
 assert(buses.count==1 && buses.unique()==&newBus);
 buses.observe(&oldBus,false);
 assert(buses.count==2 && !buses.unique());
 dbhw::ActiveInterfaces<int> detached;
 detached.observe(&oldBus,true);
 assert(detached.count==0 && !detached.unique());
 // A stopped initialized endpoint must not remain permanently ineligible.
 assert(endpointRefresh(true,true,true,true,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,false,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,true,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,false,true,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,true,true,true,true)==Refresh::Retire);
 // In-flight startup probes retain their window across bus-only changes.
 assert(endpointRefresh(false,false,true,false,false,false,true,true)==Refresh::Keep);
 assert(endpointRefresh(false,false,false,false,false,false,true,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,false,true,false,false,true,true,true)==Refresh::Retire);
 assert(endpointRefresh(true,false,true,false,true,false,true,true)==Refresh::Reprobe);
 // Repeated updates cannot renew an exhausted startup budget on unchanged hardware.
 assert(endpointRefresh(false,true,true,true,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,false,true,true,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,true,false,true,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,true,true,false,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,false,true,true,false,true,true)==Refresh::Retire);
 // Integration of the actual usability and refresh policies: an inactive child
 // remains unchanged across repeated native updates; no fresh startup window.
 bool bound=false;
 for(unsigned update=0;update<100;update++){
  bool supported=dbhw::interfaceUsable(1,true,true,true);
  bool sameInterface=bound?supported:!supported;
  assert(endpointRefresh(false,true,true,sameInterface,false,false,true)==Refresh::Keep);
 }
 // A replacement bus can need a fresh probe even if the range survived.
 assert(endpointRefresh(true,false,true,false,true,false,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,true,true,true,true,false,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,true,true,false,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(true,true,true,false,true,false,false)==Refresh::Keep);
 assert(endpointRefresh(true,true,false,false,true,false,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(true,true,true,false,true,true,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,true,true,false,true,false,false,false,true)==Refresh::Reprobe);
 // HDR recovery also requires a usable changed bus before renewing a
 // failed unpublished probe on an unchanged display binding.
 assert(endpointRefresh(false,true,true,false,false,false,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,true,true,false,false,false,true,false,false)==Refresh::Keep);
 // Normal mode may recover a lost range only on the same display binding.
 for(unsigned state=0;state<64;state++){
  bool initialized=state&1, stopped=state&2, sameBinding=state&4;
  bool sameInterface=state&8, sameRange=state&16, foreign=state&32;
  auto expected=initialized && sameBinding && !sameRange && !foreign ? Refresh::Reprobe : Refresh::Keep;
  assert(endpointRefresh(initialized,stopped,sameBinding,sameInterface,sameRange,foreign,false)==expected);
 }
 // First probe failed, then the same display gained a different usable bus.
 assert(endpointRefresh(false,true,true,false,false,false,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,true,true,true,false,false,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,false,true,false,false,false,false,true,true)==Refresh::Keep);
 puts("endpoint refresh: returning interfaces, native ownership and bounded startup policy passed");
}
