#pragma once
namespace dbhw {
inline bool interfaceUsable(unsigned count,bool present,bool inactive,bool simple){
 return count==1 && present && !inactive && simple;
}
enum class Refresh { Keep, Retire, Reprobe };
// Snapshot and slot are compared under the framebuffer gate and queue lock.
// Startup failures on unchanged resources retain their bounded probe window.
inline Refresh endpointRefresh(bool initialized,bool stopped,bool sameBinding,
 bool sameInterface,bool sameRange,bool foreignBrightness,bool enabled){
 if(!enabled)return Refresh::Keep;
 if(foreignBrightness)return Refresh::Retire;
 if(!sameBinding || !sameInterface || (initialized && (stopped || !sameRange)))
  return Refresh::Reprobe;
 return Refresh::Keep;
}
}
