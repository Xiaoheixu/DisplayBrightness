#pragma once
namespace dbhw {
template<class Service> struct ActiveInterfaces {
 unsigned count=0;
 Service *first=nullptr;
 void observe(Service *service,bool inactive){
  if(!service || inactive)return;
  if(count++==0)first=service;
 }
 Service *unique()const{return count==1?first:nullptr;}
};
inline bool interfaceUsable(unsigned count,bool present,bool inactive,bool simple){
 return count==1 && present && !inactive && simple;
}
enum class Refresh { Keep, Retire, Reprobe };
// Snapshot and slot are compared under the framebuffer gate and queue lock.
// Startup failures on unchanged resources retain their bounded probe window.
inline Refresh endpointRefresh(bool initialized,bool stopped,bool sameBinding,
 bool sameInterface,bool sameRange,bool foreignBrightness,bool enabled,bool bootstrapPending=false,
 bool usableInterface=false){
 // Default recovery stays on the same display binding. A transport change
 // only warrants a re-probe once one usable interface is present again.
 if(!enabled){
  bool needsProbe=!sameRange || (usableInterface && (!sameInterface || stopped));
  bool replacedAfterFailure=!initialized && stopped && sameBinding &&
   usableInterface && !sameInterface;
  return !foreignBrightness && ((initialized && sameBinding && needsProbe) || replacedAfterFailure)
   ? Refresh::Reprobe : Refresh::Keep;
 }
 if(foreignBrightness)return Refresh::Retire;
 if(!sameBinding || (!sameInterface && (initialized || (!bootstrapPending && usableInterface))) ||
    (initialized && (stopped || !sameRange)))
  return Refresh::Reprobe;
 return Refresh::Keep;
}
}
