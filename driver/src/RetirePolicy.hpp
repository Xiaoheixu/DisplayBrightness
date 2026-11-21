#pragma once
namespace dbhw {
// A disconnected busy endpoint will become reclaimable after its callback
// returns. Retain new bootstrap tickets until that cleanup event happens.
inline bool retirementExpected(bool occupied,bool inactive,bool retiring){
 return occupied && (inactive || retiring);
}
// Queue held. The retire worker waits for callback quiescence before reuse.
inline bool markInactiveForRetirement(bool inactive,bool &retiring){
 if(!inactive || retiring)return false;
 retiring=true;return true;
}
inline bool canRetire(bool occupied,bool inactive,bool busy,bool retiring,bool parked=false){
 return occupied && inactive && (!busy || parked) && !retiring;
}
}
