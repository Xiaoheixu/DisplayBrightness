#pragma once
namespace dbstate {
enum class Offer { Inserted,Existing,Full };
// Caller serializes access. Insertion acquires refs externally, take transfers them.
template<class T,unsigned N> struct BootstrapPending {
 struct Ticket {T *display=nullptr,*frame=nullptr;bool recovery=false;};
 Ticket entries[N];
 Offer offer(T *d,T *f,bool recovery=false){
  if(!d||!f)return Offer::Full;
  for(auto &e:entries)if(e.display==d&&e.frame==f){
   e.recovery=e.recovery||recovery;
   return Offer::Existing;
  }
  for(auto &e:entries)if(!e.display){e.display=d;e.frame=f;e.recovery=recovery;return Offer::Inserted;}
  return Offer::Full;
 }
 bool recoveryPending(T *d,T *f)const{
  if(!d||!f)return false;
  for(const auto &e:entries)if(e.display==d&&e.frame==f)return e.recovery;
  return false;
 }
 Ticket peek(unsigned i)const{return i<N?entries[i]:Ticket{};}
 Ticket take(unsigned i){if(i>=N)return Ticket{};auto t=entries[i];entries[i]=Ticket{};return t;}
 Ticket takePair(T *d,T *f){
  if(!d||!f)return Ticket{};
  for(unsigned i=0;i<N;i++)if(entries[i].display==d&&entries[i].frame==f)return take(i);
  return Ticket{};
 }
 Ticket takeMatching(unsigned i,T *d,T *f){
  if(i>=N || !d || !f || entries[i].display!=d || entries[i].frame!=f)return Ticket{};
  return take(i);
 }
};
}
