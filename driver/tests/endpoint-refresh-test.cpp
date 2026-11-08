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
 // A stopped initialized endpoint must not remain permanently ineligible.
 assert(endpointRefresh(true,true,true,true,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,false,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,true,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,false,true,true,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(true,false,true,true,true,true,true)==Refresh::Retire);
 // Repeated updates cannot renew an exhausted startup budget on unchanged hardware.
 assert(endpointRefresh(false,true,true,true,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,false,true,true,false,false,true)==Refresh::Keep);
 assert(endpointRefresh(false,true,false,true,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,true,true,false,false,false,true)==Refresh::Reprobe);
 assert(endpointRefresh(false,false,true,true,false,true,true)==Refresh::Retire);
 // Integration of the actual usability and refresh policies: an inactive child
 // remains unchanged across repeated native updates; no fresh startup window.
 bool bound=false;
 for(unsigned update=0;update<100;update++){
  bool supported=dbhw::interfaceUsable(1,true,true,true);
  bool sameInterface=bound?supported:!supported;
  assert(endpointRefresh(false,true,true,sameInterface,false,false,true)==Refresh::Keep);
 }
 // No opt-in: every slot/resource state keeps the existing endpoint policy.
 for(unsigned state=0;state<64;state++){
  assert(endpointRefresh(state&1,state&2,state&4,state&8,state&16,state&32,false)==Refresh::Keep);
 }
 puts("endpoint refresh opt-in: disabled for all64 states; enabled recovery regressions passed");
}
