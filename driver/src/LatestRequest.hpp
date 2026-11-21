#pragma once
#include <stdint.h>
namespace dbhw {
struct LatestRequest {
 bool busy=false,pending=false,failed=false,stopped=false;
 bool waitingForRetry=false;
 static constexpr unsigned maxAutomaticFailureRetries=2;
 unsigned desired=0;
 unsigned activeTarget=0;
 unsigned automaticFailureRetries=0;
 bool submit(unsigned value,bool &schedule){
  schedule=false;if(stopped || value>100)return false;
  desired=value;pending=true;
  if(!busy){automaticFailureRetries=0;busy=true;schedule=true;}
  return true;
 }
 bool refreshAfterRead(uint64_t latest,unsigned &target,uint64_t &sequence){
  if(stopped || !busy)return false;
  if(pending){target=take();sequence=latest;}
  return true;
 }
 bool hasNewTarget()const{return pending && desired!=activeTarget;}
 template<class Cancel> bool advanceRetry(Cancel cancel){
  if(!waitingForRetry || !hasNewTarget())return false;
  waitingForRetry=false;
  // If dispatch already started, its callback consumes the latest target.
  return cancel();
 }
 unsigned take(){
  if(hasNewTarget())automaticFailureRetries=0;
  pending=false;
  activeTarget=desired;
  return activeTarget;
 }
 bool complete(bool success,bool boundFailureRetries=false,unsigned retryLimit=maxAutomaticFailureRetries){
  failed=!success;
  if(success)automaticFailureRetries=0;
  else if(!boundFailureRetries)pending=true;
  else if(!hasNewTarget()){
   // Same-target feedback must not keep an exhausted request alive.
   pending=automaticFailureRetries<retryLimit;
   if(pending)++automaticFailureRetries;
  }
  if(pending && !stopped)return true;
  busy=false;return false;
 }
 // Never turn a completed startup placeholder into a brightness write.
 bool defer(bool bootstrapCompleted=false){
  waitingForRetry=false;
  if(stopped)return false;
  if(bootstrapCompleted)return complete(true);
  pending=true;return true;
 }
 void stop(){stopped=true;pending=false;busy=false;failed=true;waitingForRetry=false;}
};
}
