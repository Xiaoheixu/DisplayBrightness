// Experimental, bounded boot-lifetime endpoints. Lilu plugin is not unloadable.
// Never hold hardwareQueue while acquiring a native framebuffer gate.
struct HardwareSlot {
 IOService *display=nullptr,*frame=nullptr,*interface=nullptr;
 IORegistryEntry *connection=nullptr;
 OSObject *boundRange=nullptr;
 thread_call_t call=nullptr;
 UInt8 boundEDID[128]={};
 dbstate::BootstrapWindow bootstrapWindow;
 dbhw::ReplyMode replyMode;
 dbstate::BootstrapAttempt bootAttempt;dbstate::BootstrapJournal bootJournal;
 bool superseded=false,recoveryProbe=false,recoveryReadmit=false;
 bool haveOriginal=false,retiring=false,reclaiming=false,terminal=false,initialized=false;
 dbhw::LatestRequest requests;
 dbhw::DisplayUpdateGuard updateGuard;
 UInt64 cycleGeneration=0,deferredCycles=0;
 bool deferred=false;
 UInt64 sequence=0,busWaitSinceNs=0;UInt16 verifiedCurrent=0,verifiedMaximum=0;
 bool haveNotified=false;UInt32 lastNotifiedPercent=0;
};
HardwareSlot hardwareSlots[dbcompat::endpointCapacity];
dbhw::BusAdmission<dbcompat::endpointCapacity> busAdmission;
IOLock *hardwareQueue=nullptr;
bool hardwareRequested=false;
thread_call_t retireCall=nullptr,admissionCall=nullptr;
using PendingBootstrap=dbstate::BootstrapPending<IOService,dbcompat::endpointCapacity>;
PendingBootstrap pendingBootstrap;
bool queueHardware(IOService *,IOService *,UInt32,bool);
void releaseBootstrapTicket(PendingBootstrap::Ticket ticket){
 if(ticket.display){ticket.frame->release();ticket.display->release();}
}
UInt64 hardwareNowNs(){UInt64 now=0,ns=0;clock_get_uptime(&now);absolutetime_to_nanoseconds(now,&ns);return ns;}
// hardwareQueue held. These describe guard state, not whether startIO is active.
void guardState(HardwareSlot *c,UInt64 now){
 c->display->setProperty("DisplayBrightnessDDCGuardState",c->updateGuard.depth?"display-update":(c->updateGuard.ready(now)?"ready":"settling"));
 c->display->setProperty("DisplayBrightnessDDCGuardGeneration",c->updateGuard.generation,64);
 c->display->setProperty("DisplayBrightnessDDCGuardUntilUptimeNs",c->updateGuard.untilNs,64);
 c->display->setProperty("DisplayBrightnessDDCDeferredCycles",c->deferredCycles,64);
}
// Stack-owned records also cover an endpoint first admitted by a nested update.
// No sleep, bus access, cancellation wait or endpoint allocation in this scope.
struct DisplayUpdateScope;
DisplayUpdateScope *displayUpdates=nullptr;
struct DisplayUpdateScope {
 IOService *display=nullptr;DisplayUpdateScope *next=nullptr;
 explicit DisplayUpdateScope(IOService *s){
  if(!hardwareRequested || !hardwareQueue || !enabled() || !pilot(s))return;
  FrameScope gate(s);if(!gate.valid(s))return;
  IOLockLock(hardwareQueue);
  display=s;next=displayUpdates;displayUpdates=this;
  for(auto &c:hardwareSlots)if(c.display==s&&!c.retiring){
   c.updateGuard.begin();guardState(&c,hardwareNowNs());break;
  }
  IOLockUnlock(hardwareQueue);
 }
 ~DisplayUpdateScope(){
  if(!display)return;
  // Balance even if originalUpdate changed identity, topology or active state.
  IOLockLock(hardwareQueue);
  auto **link=&displayUpdates;while(*link&&*link!=this)link=&(*link)->next;
  if(*link)*link=next;
  for(auto &c:hardwareSlots)if(c.display==display&&!c.retiring){
   c.updateGuard.end(hardwareNowNs());guardState(&c,hardwareNowNs());break;
  }
  IOLockUnlock(hardwareQueue);
 }
};
// hardwareQueue held. Preserve latest target and busy ownership until resumed.
void deferHardware(HardwareSlot *c,bool bootstrapCompleted=false){
 ++c->deferredCycles;
 if(!c->requests.defer(bootstrapCompleted)){guardState(c,hardwareNowNs());return;}
 UInt64 now=hardwareNowNs(),remaining=c->updateGuard.untilNs>now?c->updateGuard.untilNs-now:0;
 UInt64 ms=c->updateGuard.depth?100:(remaining/1000000ULL+(remaining%1000000ULL!=0));
 if(!ms)ms=1;
 uint64_t deadline;clock_interval_to_deadline(static_cast<UInt32>(ms),kMillisecondScale,&deadline);
 guardState(c,now);thread_call_enter_delayed(c->call,deadline);
}
// Transport/publication callers hold the native gate. Retry polling only reads
// guard state under the queue; it never authorizes transport by itself.
bool cyclePermitted(HardwareSlot *c){
 IOLockLock(hardwareQueue);
 bool ok=c->updateGuard.permits(hardwareNowNs(),c->cycleGeneration);
 if(!ok)c->deferred=true;
 IOLockUnlock(hardwareQueue);return ok;
}
void admissionWorker(thread_call_param_t,thread_call_param_t){
 for(unsigned i=0;i<dbcompat::endpointCapacity;i++){
  IOLockLock(hardwareQueue);
  auto observed=pendingBootstrap.peek(i);
  if(observed.display){observed.display->retain();observed.frame->retain();}
  IOLockUnlock(hardwareQueue);
  if(!observed.display)continue;
  PendingBootstrap::Ticket invalid;
  {
   FrameScope gate(observed.display);
   bool valid=gate.valid(observed.display)&&gate.frameService==observed.frame;
   IOLockLock(hardwareQueue);
   auto current=pendingBootstrap.peek(i);
   bool matches=current.display==observed.display && current.frame==observed.frame;
   if(matches && !valid)invalid=pendingBootstrap.takeMatching(i,observed.display,observed.frame);
   IOLockUnlock(hardwareQueue);
   // Leave the ticket in the queue until admission actually succeeds.
   if(matches && valid)queueHardware(observed.display,observed.frame,0,true);
  }
  releaseBootstrapTicket(invalid);
  releaseBootstrapTicket(observed);
 }
}
// hardwareQueue held. Existing tickets already own their reference pair.
bool rememberBootstrap(IOService *s,IOService *fb,bool recovery){
 auto result=pendingBootstrap.offer(s,fb,recovery);
 if(result==dbstate::Offer::Inserted){s->retain();fb->retain();}
 return result!=dbstate::Offer::Full;
}

void retireWorker(thread_call_param_t,thread_call_param_t){
 // Runs independently of framebuffer gates. Retiring slots cannot accept work.
 for(auto &c:hardwareSlots){
  IOLockLock(hardwareQueue);
  bool retire=c.retiring && !c.reclaiming;
  if(retire)c.reclaiming=true;
  thread_call_t call=c.call;
  IOLockUnlock(hardwareQueue);
  if(!retire)continue;
  // No queue, bus or native framebuffer gate held while waiting.
  if(call)thread_call_cancel_wait(call);
  IOLockLock(hardwareQueue);
  IOService *display=c.display,*frame=c.frame,*interface=c.interface;
  IORegistryEntry *connection=c.connection;c.connection=nullptr;
  OSObject *range=c.boundRange;c.boundRange=nullptr;
  busAdmission.forget(static_cast<unsigned>(&c-hardwareSlots));
  c.display=nullptr;c.frame=nullptr;c.interface=nullptr;
  c.bootstrapWindow=dbstate::BootstrapWindow{};c.replyMode=dbhw::ReplyMode{};
  c.bootAttempt=dbstate::BootstrapAttempt{};c.bootJournal=dbstate::BootstrapJournal{};
  c.updateGuard=dbhw::DisplayUpdateGuard{};c.cycleGeneration=0;c.deferredCycles=0;c.deferred=false;
  c.requests=dbhw::LatestRequest{};c.haveOriginal=false;c.terminal=false;c.initialized=false;
  c.sequence=0;c.busWaitSinceNs=0;c.verifiedCurrent=0;c.verifiedMaximum=0;
  c.haveNotified=false;c.lastNotifiedPercent=0;c.superseded=false;
  c.recoveryProbe=false;c.recoveryReadmit=false;
  // Keep the completed thread_call object stable for reuse; never free in flight.
  c.retiring=false;c.reclaiming=false;
  IOLockUnlock(hardwareQueue);
  if(range)range->release();if(interface)interface->release();if(connection)connection->release();if(frame)frame->release();if(display)display->release();
  // Admission resumes only after the old callback is fully quiescent.
  if(admissionCall)thread_call_enter(admissionCall);
 }
}

// Caller holds the native framebuffer gate. Ownership requires the exact
// brightness object in this snapshot, not just an old marker on the service.
dbhw::BrightnessProvider brightnessProvider(IOService *s,OSObject **ownedRange=nullptr){
 if(ownedRange)*ownedRange=nullptr;
 OSObject *owner=s->copyProperty(ownerKey);
 OSObject *property=s->copyProperty("IODisplayParameters");
 auto *params=OSDynamicCast(OSDictionary,property);
 auto *range=OSDynamicCast(OSDictionary,owner);
 auto *current=params?params->getObject("brightness"):nullptr;
 auto provider=dbhw::brightnessProvider(current!=nullptr,current==owner,
  range && range->getObject("DisplayBrightnessSynthetic")==kOSBooleanTrue);
 if(ownedRange && provider==dbhw::BrightnessProvider::Owned){
  *ownedRange=owner;owner=nullptr;
 }
 if(property)property->release();
 if(owner)owner->release();
 return provider;
}
bool hardwarePilot(IOService *s);
bool endpointEDIDMatches(HardwareSlot *c){
 auto *o=c->display->copyProperty("IODisplayEDID");auto *e=OSDynamicCast(OSData,o);
 bool ok=e && dbcompat::sameBase(c->boundEDID,static_cast<const UInt8 *>(e->getBytesNoCopy()),e->getLength());
 if(o)o->release();return ok;
}
bool captureEndpointEDID(IOService *s,UInt8 (&bound)[128]){
 auto *o=s->copyProperty("IODisplayEDID");auto *e=OSDynamicCast(OSData,o);
 bool ok=e && dbcompat::validBase(static_cast<const UInt8 *>(e->getBytesNoCopy()),e->getLength());
 if(ok)memcpy(bound,e->getBytesNoCopy(),128);
 if(o)o->release();return ok;
}
// Scan only the bound framebuffer's direct IOService children. The caller
// holds its native gate; an inactive service can still be present during teardown.
IOService *copyUniqueActiveInterface(IOService *frame,unsigned &count,bool *enumerated=nullptr){
 dbhw::ActiveInterfaces<IOService> interfaces;
 auto *it=frame->getChildIterator(gIOServicePlane);
 if(enumerated)*enumerated=it!=nullptr;
 if(it){
  while(auto *child=it->getNextObject()){
   auto *service=OSDynamicCast(IOService,child);
   if(service && hasClass(service,"IOFramebufferI2CInterface"))
    interfaces.observe(service,service->isInactive());
  }
 }
 count=interfaces.count;
 auto *selected=interfaces.unique();
 if(selected)selected->retain();
 if(it)it->release();
 return selected;
}
OSNumber *copyTransactionTypes(IOService *interface){
 auto *property=interface?interface->copyProperty(kIOI2CTransactionTypesKey):nullptr;
 auto *types=OSDynamicCast(OSNumber,property);
 if(property && !types)property->release();
 return types;
}
bool supportsSimple(OSNumber *types){
 return types && (types->unsigned32BitValue()&(1U<<kIOI2CSimpleTransactionType));
}
bool hardwareLive(HardwareSlot *c,FrameScope &gate,bool bootstrapRead=false,bool *interfaceChanged=nullptr){
 if(interfaceChanged)*interfaceChanged=false;
 if(!c->interface || !gate.valid(c->display) ||
    gate.connection!=c->connection || gate.frameService!=c->frame)return false;
 IOLockLock(hardwareQueue);bool retiring=c->retiring||c->reclaiming;IOLockUnlock(hardwareQueue);
 if(retiring)return false;
 OSObject *range=nullptr;
 auto provider=brightnessProvider(c->display,&range);
 bool allowed=dbhw::allowsTransport(provider,c->initialized,range && range==c->boundRange,bootstrapRead);
 if(range)range->release();
 if(!allowed)return false;
 unsigned count=0;
 auto *selected=copyUniqueActiveInterface(c->frame,count);
 bool same=dbcompat::sameTopology(c->connection,gate.connection,c->frame,gate.frameService,count,selected==c->interface);
 if(selected)selected->release();
 if(!endpointEDIDMatches(c) || !gate.valid(c->display))return false;
 auto *parent=c->interface->copyParentEntry(gIOServicePlane);
 bool ok=same && parent==c->frame && !c->interface->isInactive();
 if(parent)parent->release();
 if(interfaceChanged)*interfaceChanged=!ok && bootstrapRead && !c->initialized;
 return ok;
}
// The caller holds bus serialization; scope releases native gate after each IO.
bool hardwareIO(HardwareSlot *c,IOI2CRequest &q,const char *operation){
 FrameScope gate(c->display);
 if(!cyclePermitted(c))return false;
 bool interfaceChanged=false;
 if(!hardwareLive(c,gate,!strcmp(operation,"get-initial-brightness"),&interfaceChanged)){
  c->terminal=!interfaceChanged;
  c->display->setProperty("DisplayBrightnessHardwareState",interfaceChanged?"waiting-interface-change":"offline-or-replaced");
  return false;
 }
 // A Set must still permit a verified readback on the bound interface.
 if(c->replyMode.select(true,c->initialized)==dbhw::ReplyKind::Framed){
  auto *property=c->interface->copyProperty(kIOI2CTransactionTypesKey);
  auto *types=OSDynamicCast(OSNumber,property);
  bool framed=types && (types->unsigned32BitValue()&(1U<<kIOI2CDDCciReplyTransactionType));
  if(property)property->release();
  auto selected=c->replyMode.select(framed,c->initialized);
  if(selected==dbhw::ReplyKind::Unavailable){
   c->terminal=true;
   c->display->setProperty("DisplayBrightnessHardwareState","reply-mode-unavailable");
   return false;
  }
  if(selected==dbhw::ReplyKind::Framed && q.replyTransactionType==kIOI2CSimpleTransactionType)
   q.replyTransactionType=kIOI2CDDCciReplyTransactionType;
 }
 // Publish the operation before entering the synchronous driver call.
 c->display->setProperty("DisplayBrightnessHardwareOperation",operation);
 if(!c->initialized){
  c->display->setProperty("DisplayBrightnessHardwareReplyType",q.replyTransactionType,32);
  UInt64 now=0,ns=0;clock_get_uptime(&now);absolutetime_to_nanoseconds(now,&ns);
  if(!c->bootstrapWindow.within(ns)){
   c->terminal=true;c->bootAttempt.phase=dbstate::BootstrapPhase::Exhausted;
   c->display->setProperty("DisplayBrightnessBootstrapState","probe-window-exhausted-reconnect-required");return false;
  }
 }
 IOReturn status=startIO(c->interface,&q);
 if(!c->initialized){c->bootAttempt.attempted=true;c->bootAttempt.transport=static_cast<UInt32>(status);c->bootAttempt.request=static_cast<UInt32>(q.result);}
 c->display->setProperty("DisplayBrightnessHardwareTransportResult",static_cast<UInt32>(status),32);
 c->display->setProperty("DisplayBrightnessHardwareRequestResult",static_cast<UInt32>(q.result),32);
 if(status!=kIOReturnSuccess || q.result!=kIOReturnSuccess){
  c->display->setProperty("DisplayBrightnessHardwareState","transport-error-stopped");return false;
 }
 return true;
}
bool hardwareReadOnce(HardwareSlot *c,dbddc::Brightness &b,const char *operation){
 if(!c->initialized){++c->bootAttempt.reads;c->bootAttempt.replyValid=false;}
 UInt8 send[5],reply[11]={0};dbddc::getBrightness(send);
 IOI2CRequest q={};q.sendAddress=0x6e;q.replyAddress=0x6f;q.replySubAddress=0x51;
 q.sendTransactionType=kIOI2CSimpleTransactionType;q.replyTransactionType=kIOI2CSimpleTransactionType;
 q.sendBuffer=reinterpret_cast<vm_address_t>(send);q.sendBytes=sizeof(send);
 q.replyBuffer=reinterpret_cast<vm_address_t>(reply);q.replyBytes=sizeof(reply);
 clock_interval_to_absolutetime_interval(bootSettings.replyMs,1000000,&q.minReplyDelay);
 bool ioOK=hardwareIO(c,q,operation);
 if(!c->initialized)c->bootAttempt.captureReply(reply,sizeof(reply),q.replyBytes);
 b=ioOK?dbddc::decodeBrightness(reply,q.replyBytes):dbddc::Brightness{};
 if(!c->initialized && c->bootAttempt.attempted){
  auto attempted=q.replyTransactionType==kIOI2CDDCciReplyTransactionType?
   dbhw::ReplyKind::Framed:dbhw::ReplyKind::Simple;
  c->replyMode.bootstrapReadFinished(attempted,b.valid);
  c->bootAttempt.replyValid=b.valid;
  c->bootAttempt.replyType=q.replyTransactionType;
 }
 if(!ioOK)return false;
 c->display->setProperty("DisplayBrightnessHardwareReply",reply,sizeof(reply));
 if(!b.valid){c->display->setProperty("DisplayBrightnessHardwareState","invalid-reply-stopped");return false;}
 c->display->setProperty("DisplayBrightnessHardwareActual",b.current,32);
 c->display->setProperty("DisplayBrightnessHardwareMaximum",b.maximum,32);
 return true;
}
bool hardwarePilot(IOService *s){return pilot(s);}
// This measures the full read phase, including retry waits and framebuffer gate.
bool hardwareRead(HardwareSlot *c,dbddc::Brightness &b,const char *operation,bool responsive=false,UInt32 captured=0){
 uint64_t begin=0,end=0,ns=0;clock_get_uptime(&begin);unsigned attempts=0;
 auto read=[&](){++attempts;b=dbddc::Brightness{};return hardwareReadOnce(c,b,operation);};
 bool ok=false;
 if(responsive){
  auto result=dbhw::responsiveRead(read,[](){IOSleep(10);},[&](){return !c->terminal&&cyclePermitted(c);},[&](){
   IOLockLock(hardwareQueue);
   bool obsolete=dbhw::differentPending(c->requests.pending,c->requests.desired,captured);
   IOLockUnlock(hardwareQueue);return obsolete;
  });
  c->superseded=result==dbhw::ReadResult::Superseded;ok=result==dbhw::ReadResult::Valid;
 }else{
  ok=dbhw::readRecovery(read,[](){IOSleep(250);},[&](){return !c->terminal&&cyclePermitted(c);});
 }
 clock_get_uptime(&end);absolutetime_to_nanoseconds(end-begin,&ns);
 bool before=!strcmp(operation,"get-before-write"),after=!strcmp(operation,"get-after-write");
 c->display->setProperty(before?"DisplayBrightnessBeforeReadMicroseconds":(after?"DisplayBrightnessAfterReadMicroseconds":"DisplayBrightnessInitialReadMicroseconds"),ns/1000,64);
 c->display->setProperty(before?"DisplayBrightnessBeforeReadAttempts":(after?"DisplayBrightnessAfterReadAttempts":"DisplayBrightnessInitialReadAttempts"),attempts,32);
 return ok;
}
bool prepareBootstrapInterface(HardwareSlot *c){
 FrameScope gate(c->display);
 if(!cyclePermitted(c))return false;
 if(!gate.valid(c->display)||!endpointEDIDMatches(c)||gate.connection!=c->connection||gate.frameService!=c->frame){c->terminal=true;return false;}
 if(brightnessProvider(c->display)==dbhw::BrightnessProvider::Foreign){
  c->terminal=true;
  c->display->setProperty("DisplayBrightnessBootstrapState","native-provider-preserved");
  return false;
 }
 unsigned count=0;
 auto *iface=copyUniqueActiveInterface(c->frame,count);
 auto *types=copyTransactionTypes(iface);
 bool supported=dbhw::interfaceUsable(count,iface!=nullptr,iface && iface->isInactive(),supportsSimple(types));
 if(types)types->release();
 if(!supported && iface){iface->release();iface=nullptr;}
 // Until publication, a transient topology change can settle within this
 // slot's existing probe window. Forget reply mode if the interface changes.
 IOLockLock(hardwareQueue);
 auto *old=c->interface;
 if(old!=iface)c->replyMode=dbhw::ReplyMode{};
 c->interface=iface;
 IOLockUnlock(hardwareQueue);
 if(old)old->release();
 c->display->setProperty("DisplayBrightnessI2CActiveCount",count,32);
 if(!supported)
  c->display->setProperty("DisplayBrightnessBootstrapState",count>1?"waiting-unambiguous-interface":"waiting-interface");
 return supported;
}
bool initializeEndpoint(HardwareSlot *c){
 c->bootAttempt.phase=dbstate::BootstrapPhase::Interface;
 if(!prepareBootstrapInterface(c))return false;
 c->bootAttempt.phase=dbstate::BootstrapPhase::Read;
 return dbstate::bootstrap([&](){
  // One read per startup visit. Release the bus before the one-second retry.
  dbddc::Brightness raw;UInt64 begin=0,end=0,ns=0;clock_get_uptime(&begin);
  hardwareReadOnce(c,raw,"get-initial-brightness");clock_get_uptime(&end);absolutetime_to_nanoseconds(end-begin,&ns);
  c->display->setProperty("DisplayBrightnessInitialReadMicroseconds",ns/1000,64);
  c->display->setProperty("DisplayBrightnessInitialReadAttempts",static_cast<UInt32>(1),32);return raw;
 },[&](unsigned value,dbddc::Brightness raw){
  if(bootSettings.getOnly){
   c->terminal=true;
   c->bootAttempt.phase=dbstate::BootstrapPhase::ReadOnly;
   c->display->setProperty("DisplayBrightnessBootstrapState","read-only-verified");
   return true;
  }
  c->bootAttempt.phase=dbstate::BootstrapPhase::Publish;
  FrameScope gate(c->display);
  if(!cyclePermitted(c))return false;
  bool interfaceChanged=false;
  if(!hardwareLive(c,gate,true,&interfaceChanged)){
   c->terminal=!interfaceChanged;
   if(interfaceChanged)c->display->setProperty("DisplayBrightnessBootstrapState","waiting-interface-change");
   return false;
  }
  publish(c->display,value);publishEDMMarker(c->display);
  OSObject *range=nullptr;
  auto provider=brightnessProvider(c->display,&range);
  if(!range){
   c->terminal=provider==dbhw::BrightnessProvider::Foreign;
   return false;
  }
  c->verifiedCurrent=raw.current;c->verifiedMaximum=raw.maximum;
  c->display->setProperty("DisplayBrightnessHardwareOriginal",raw.current,32);
  c->display->setProperty("DisplayBrightnessHardwareOriginalMaximum",raw.maximum,32);c->haveOriginal=true;
  c->display->setProperty("DisplayBrightnessBootstrapState","published-from-hardware");
  IOLockLock(hardwareQueue);
  bool commit=range && !c->retiring && !c->reclaiming;
  if(commit){c->boundRange=range;c->initialized=true;}
  IOLockUnlock(hardwareQueue);
  if(!commit){if(range)range->release();c->terminal=true;return false;}
  c->bootAttempt.phase=dbstate::BootstrapPhase::Ready;
  return true;
 });
}
bool hardwareCycle(HardwareSlot *c,UInt32 &desired,UInt64 &sequence){
 if(!c->initialized || bootSettings.getOnly)return false;
 // Re-read on every cycle. A previous reply/cache never authorizes this write.
 dbddc::Brightness before;
 if(!hardwareRead(c,before,"get-before-write",true,desired))return false;
 // Fresh read completed. Adopt the latest input before choosing this write.
 // No native framebuffer or bus operation while holding hardwareQueue.
 IOLockLock(hardwareQueue);
 bool selected=c->requests.refreshAfterRead(c->sequence,desired,sequence);
 IOLockUnlock(hardwareQueue);
 if(!selected)return false;
 c->display->setProperty("DisplayBrightnessHardwareDesired",desired,32);
 if(!c->haveOriginal){
  c->display->setProperty("DisplayBrightnessHardwareOriginal",before.current,32);
  c->display->setProperty("DisplayBrightnessHardwareOriginalMaximum",before.maximum,32);
  c->haveOriginal=true;
 }
 UInt16 target=0;
 if(!dbhw::target(before,desired,target)){
  c->display->setProperty("DisplayBrightnessHardwareState","invalid-target-stopped");return false;
 }
 c->display->setProperty("DisplayBrightnessHardwareTarget",target,32);
 if(before.current!=target){
  UInt8 packet[7];if(!dbddc::setBrightness(before,target,packet))return false;
  IOI2CRequest q={};q.sendAddress=0x6e;q.sendTransactionType=kIOI2CSimpleTransactionType;
  q.replyTransactionType=kIOI2CNoTransactionType;
  q.sendBuffer=reinterpret_cast<vm_address_t>(packet);q.sendBytes=sizeof(packet);
  uint64_t begin=0,end=0,ns=0;clock_get_uptime(&begin);
  bool wrote=hardwareIO(c,q,"set-brightness");
  clock_get_uptime(&end);absolutetime_to_nanoseconds(end-begin,&ns);
  c->display->setProperty("DisplayBrightnessSetMicroseconds",ns/1000,64);
  if(!wrote)return false;
  // No framebuffer gate is held during monitor settling. Bus serialization stays.
  IOSleep(bootSettings.settleMs);
 }
 bool readOK=true;
 bool verified=dbhw::verifyTarget(before,target,[&](dbddc::Brightness &after){
  readOK=hardwareRead(c,after,"get-after-write",true,desired);return readOK;
 },[&](dbddc::Brightness after){
  c->verifiedCurrent=after.current;c->verifiedMaximum=after.maximum;
  c->display->setProperty("DisplayBrightnessHardwareVerified",after.current,32);
  c->display->setProperty("DisplayBrightnessHardwareState",before.current==target?"verified-already-target":"verified-readback");
 });
 if(!verified&&readOK)c->display->setProperty("DisplayBrightnessHardwareState","readback-mismatch-stopped");
 return verified;
}

void syncVerifiedState(HardwareSlot *c,UInt64 sequence){
 unsigned value=0;
 if(!dbstate::percent(true,c->verifiedCurrent,c->verifiedMaximum,value))return;
 bool changed=false;
 {
  FrameScope gate(c->display);
  if(!cyclePermitted(c)||!hardwareLive(c,gate))return;
  // Same lock order as submission: native framebuffer gate, then queue lock.
  IOLockLock(hardwareQueue);
  if(dbstate::mayPublish(sequence,c->sequence,c->requests.pending,true)){
   OSObject *o=c->display->copyProperty(ownerKey);
   auto *range=OSDynamicCast(OSDictionary,o);
   auto *n=range?OSDynamicCast(OSNumber,range->getObject("value")):nullptr;
   if(n){n->setValue(value);changed=true;}
   if(o)o->release();
  }
  IOLockUnlock(hardwareQueue);
 }
 if(changed && bootSettings.notify && dbstate::shouldNotify(c->haveNotified,c->lastNotifiedPercent,value)){
  // Current AppleMCCS producer sends the changed key, zero padded to 16 bytes.
  char key[16]="brightness";
  IOReturn status=c->display->messageClients(kIOMessageServicePropertyChange,key,sizeof(key));
  if(status==kIOReturnSuccess){c->haveNotified=true;c->lastNotifiedPercent=value;}
  c->display->setProperty("DisplayBrightnessStateSyncPercent",value,32);
  c->display->setProperty("DisplayBrightnessStateSyncSequence",sequence,64);
  c->display->setProperty("DisplayBrightnessStateSyncMessageResult",static_cast<UInt32>(status),32);
 }
}
// Startup-only diagnostics: bounded history, emitted after releasing the bus.
void recordBootstrapAttempt(HardwareSlot *c,bool ok,UInt64 started,UInt64 finished){
 absolutetime_to_nanoseconds(started,&c->bootAttempt.startedNs);
 absolutetime_to_nanoseconds(finished,&c->bootAttempt.finishedNs);c->bootAttempt.success=ok;
 bool recorded=c->bootJournal.append(c->bootAttempt);
 if(!recorded)return;
 if(dbBootLogEnabled && !c->recoveryProbe)
  IOLog("DisplayBrightness DBBOOT stage=probe-result display=%llu round=%u phase=%s ok=%d attempted=%d validReply=%d replyType=%u transport=0x%x request=0x%x\n",
        c->display->getRegistryEntryID(),c->bootJournal.total,dbstate::bootstrapPhaseText(c->bootAttempt.phase),
        ok,c->bootAttempt.attempted,c->bootAttempt.replyValid,c->bootAttempt.replyType,c->bootAttempt.transport,c->bootAttempt.request);
 if(bootSettings.trace || (bootSettings.hdrTrace && c->recoveryProbe))IOLog("DisplayBrightness: bootstrap display=%llu round=%u phase=%s ok=%u reads=%u transport=0x%x request=0x%x uptimeNs=%llu..%llu\n",c->display->getRegistryEntryID(),c->bootJournal.total,dbstate::bootstrapPhaseText(c->bootAttempt.phase),ok,c->bootAttempt.reads,c->bootAttempt.transport,c->bootAttempt.request,c->bootAttempt.startedNs,c->bootAttempt.finishedNs);
 c->display->setProperty("DisplayBrightnessBootstrapAttemptCount",c->bootJournal.total,32);
 c->display->setProperty("DisplayBrightnessBootstrapOmittedCount",c->bootJournal.dropped,32);
 auto *array=OSArray::withCapacity(c->bootJournal.count);if(!array)return;bool valid=true;
 auto put=[](OSDictionary *d,const char *key,UInt64 value){auto *n=OSNumber::withNumber(value,64);bool ok=n&&d->setObject(key,n);if(n)n->release();return ok;};
 for(unsigned i=0;i<c->bootJournal.count&&valid;i++){
  const auto &a=c->bootJournal.attempts[i];auto *d=OSDictionary::withCapacity(15);if(!d){valid=false;break;}
  auto *phase=OSString::withCString(dbstate::bootstrapPhaseText(a.phase));
  valid=phase&&d->setObject("phase",phase)&&put(d,"round",a.ordinal)&&put(d,"startedUptimeNs",a.startedNs)&&put(d,"finishedUptimeNs",a.finishedNs)&&put(d,"reads",a.reads)&&put(d,"transport",a.transport)&&put(d,"request",a.request)&&put(d,"replyType",a.replyType)&&d->setObject("transportAttempted",a.attempted?kOSBooleanTrue:kOSBooleanFalse)&&d->setObject("replyValid",a.replyValid?kOSBooleanTrue:kOSBooleanFalse)&&d->setObject("success",a.success?kOSBooleanTrue:kOSBooleanFalse)&&put(d,"reportedReplyBytes",a.reportedReplyBytes)&&put(d,"capturedReplyBytes",a.capturedReplyBytes);
  auto *raw=OSData::withBytes(a.reply,a.capturedReplyBytes);
  valid=valid&&raw&&d->setObject("reply",raw)&&array->setObject(d);if(raw)raw->release();
  if(phase)phase->release();d->release();
 }
 if(valid)c->display->setProperty("DisplayBrightnessBootstrapHistory",array);array->release();
}
// All waiters are bounded by the slot count. They race for the token under
// hardwareQueue after wakeup and park again if another endpoint owns it.
void releaseHardwareBus(unsigned slot){
 IOLockLock(hardwareQueue);
 uint32_t ready=busAdmission.release(slot);
 for(unsigned i=0;i<dbcompat::endpointCapacity;i++){
  auto &waiter=hardwareSlots[i];
  if((ready&(uint32_t{1}<<i)) && waiter.display && !waiter.retiring &&
     !waiter.reclaiming && waiter.requests.busy && waiter.requests.pending &&
     !waiter.requests.stopped && waiter.call)
   thread_call_enter(waiter.call);
 }
 IOLockUnlock(hardwareQueue);
}
void hardwareWorker(thread_call_param_t arg,thread_call_param_t){
 auto *c=static_cast<HardwareSlot *>(arg);
 unsigned slot=static_cast<unsigned>(c-hardwareSlots);
 uint64_t started=0,finished=0,elapsed=0,startedNs=0;
 clock_get_uptime(&started);absolutetime_to_nanoseconds(started,&startedNs);
 IOLockLock(hardwareQueue);
 if(!c->display || c->retiring || c->reclaiming || c->requests.stopped ||
    !c->requests.busy || !c->requests.pending){
  IOLockUnlock(hardwareQueue);return;
 }
 if(c->display->isInactive()){
  c->requests.stop();busAdmission.forget(slot);
  bool retire=retireCall && dbhw::markInactiveForRetirement(true,c->retiring);
  IOLockUnlock(hardwareQueue);
  if(retire)thread_call_enter(retireCall);
  return;
 }
 c->requests.waitingForRetry=false;
 if(!c->updateGuard.ready(startedNs)){deferHardware(c);IOLockUnlock(hardwareQueue);return;}
 c->cycleGeneration=c->updateGuard.generation;c->deferred=false;guardState(c,startedNs);
 bool initialized=c->initialized;
 auto admission=dbhw::admitVisit(busAdmission,slot,c->bootstrapWindow,initialized,startedNs);
 if(admission==dbhw::VisitAdmission::Parked){
  if(!c->busWaitSinceNs)c->busWaitSinceNs=startedNs;
  c->display->setProperty("DisplayBrightnessHardwareState","waiting-for-bus");
  // Leave the target pending. No sleeping callback or polling timer.
  IOLockUnlock(hardwareQueue);return;
 }
 UInt64 waitSince=c->busWaitSinceNs;c->busWaitSinceNs=0;
 UInt32 desired=c->requests.take();UInt64 sequence=c->sequence;
 IOLockUnlock(hardwareQueue);
 c->display->setProperty("DisplayBrightnessHardwareDesired",desired,32);
 UInt64 waited=waitSince && startedNs>=waitSince?startedNs-waitSince:0;
 c->display->setProperty("DisplayBrightnessBusWaitMicroseconds",waited/1000,64);
 // The token serializes whole transactions. It cannot cancel an in-flight IO.
 c->terminal=false;c->superseded=false;
 const char *metrics[]={"DisplayBrightnessBeforeReadMicroseconds","DisplayBrightnessAfterReadMicroseconds","DisplayBrightnessBeforeReadAttempts","DisplayBrightnessAfterReadAttempts","DisplayBrightnessSetMicroseconds"};
 for(const char *key:metrics)c->display->setProperty(key,static_cast<UInt64>(0),64);
 if(!initialized)c->bootAttempt=dbstate::BootstrapAttempt{};
 bool bootAllowed=admission==dbhw::VisitAdmission::Ready;
 bool ok=false;
 if(!bootAllowed){
  c->terminal=true;c->bootAttempt.phase=dbstate::BootstrapPhase::Exhausted;
  c->display->setProperty("DisplayBrightnessBootstrapState","probe-window-exhausted-reconnect-required");
 }else{
  if(dbBootLogEnabled && !initialized && !c->recoveryProbe)
   IOLog("DisplayBrightness DBBOOT stage=probe-start display=%llu\n",c->display->getRegistryEntryID());
  ok=initialized?hardwareCycle(c,desired,sequence):initializeEndpoint(c);
 }
 releaseHardwareBus(slot);
 clock_get_uptime(&finished);absolutetime_to_nanoseconds(finished-started,&elapsed);
 if(!initialized&&bootAllowed&&!c->deferred)recordBootstrapAttempt(c,ok,started,finished);
 if(!initialized&&c->deferred&&!c->bootAttempt.attempted&&c->bootstrapWindow.rounds)
  --c->bootstrapWindow.rounds;
 c->display->setProperty("DisplayBrightnessHardwareCycleMicroseconds",elapsed/1000,64);
 c->display->setProperty("DisplayBrightnessHardwareCycleSequence",sequence,64);
 if(ok && c->initialized)syncVerifiedState(c,sequence);
 // Deadline bounds new startup attempts, not an in-flight synchronous AMD call.
 UInt64 finishedNs=0;absolutetime_to_nanoseconds(finished,&finishedNs);
 if(!initialized&&!ok&&!c->terminal&&!c->deferred&&!c->bootstrapWindow.maySchedule(finishedNs)){
  c->terminal=true;
  c->display->setProperty("DisplayBrightnessBootstrapState","probe-window-exhausted-reconnect-required");
 }
 if(dbBootLogEnabled && !initialized && !ok && c->terminal && !c->recoveryProbe)
  IOLog("DisplayBrightness DBBOOT stage=probe-stopped display=%llu\n",c->display->getRegistryEntryID());
 bool live=!c->terminal && hardwarePilot(c->display);
 IOLockLock(hardwareQueue);
 live=live && !c->retiring && !c->reclaiming;
 if(live&&(c->deferred||!c->updateGuard.permits(hardwareNowNs(),c->cycleGeneration))){
  deferHardware(c,!initialized&&c->initialized);IOLockUnlock(hardwareQueue);return;
 }
 bool retireNeeded=false;
 if(!live){
  c->requests.stop();
  if(retireCall)retireNeeded=dbhw::markInactiveForRetirement(c->display->isInactive(),c->retiring);
 }
 bool newTarget=c->requests.hasNewTarget();
 bool yielded=c->superseded&&live&&newTarget;
 if(yielded)c->display->setProperty("DisplayBrightnessHardwareState","superseded-unverified");
 const bool reschedule=live && c->requests.complete(ok||yielded,initialized,bootSettings.retries);
 if(live && initialized && !ok && !yielded && !reschedule)
  c->display->setProperty("DisplayBrightnessHardwareState","request-retries-exhausted");
 if(reschedule){
  bool coalesce=ok || (initialized && newTarget);
  unsigned delay=coalesce?bootSettings.coalesceMs:(initialized?2000:c->bootstrapWindow.cadenceMs);
  uint64_t deadline;clock_interval_to_deadline(delay,kMillisecondScale,&deadline);
  if(!coalesce)c->display->setProperty("DisplayBrightnessHardwareState",initialized?"recovering-read-before-write":"probe-cooldown");
  c->requests.waitingForRetry=initialized && !coalesce;
  thread_call_enter_delayed(c->call,deadline);
 }
 IOLockUnlock(hardwareQueue);
 if(retireNeeded)thread_call_enter(retireCall);
}
void endpointSnapshot(IOService *s,const char *reason,unsigned interfaces){
 // Caller holds hardwareQueue; snapshots carry registry IDs, never kernel pointers.
 auto *d=OSDictionary::withCapacity(5);if(!d)return;
 auto *why=OSString::withCString(reason);
 if(why){d->setObject("reason",why);why->release();}
 auto put=[](OSDictionary *dict,const char *key,UInt64 value){
  auto *n=OSNumber::withNumber(value,64);if(n){dict->setObject(key,n);n->release();}
 };
 put(d,"event",nextEvent());put(d,"interfaceCount",interfaces);
 put(d,"requestDisplay",s->getRegistryEntryID());
 for(unsigned i=0;i<dbcompat::endpointCapacity;i++){
  auto &c=hardwareSlots[i];auto *item=OSDictionary::withCapacity(9);if(!item)continue;
  put(item,"display",c.display?c.display->getRegistryEntryID():0);
  put(item,"frame",c.frame?c.frame->getRegistryEntryID():0);
  put(item,"interface",c.interface?c.interface->getRegistryEntryID():0);
  item->setObject("retiring",c.retiring?kOSBooleanTrue:kOSBooleanFalse);
  item->setObject("inactive",c.display && c.display->isInactive()?kOSBooleanTrue:kOSBooleanFalse);
  item->setObject("busy",c.requests.busy?kOSBooleanTrue:kOSBooleanFalse);
  item->setObject("pending",c.requests.pending?kOSBooleanTrue:kOSBooleanFalse);
  item->setObject("failed",c.requests.failed?kOSBooleanTrue:kOSBooleanFalse);
  item->setObject("waitingForRetry",c.requests.waitingForRetry?kOSBooleanTrue:kOSBooleanFalse);
  put(item,"desired",c.requests.desired);
  char key[16];snprintf(key,sizeof(key),"slot%u",i);
  d->setObject(key,item);item->release();
 }
 s->setProperty("DisplayBrightnessEndpointSnapshot",d);d->release();
}
// Called with the native framebuffer gate and queue held; never acquires a gate.
dbhw::Refresh refreshEndpoint(HardwareSlot *c,IOService *s,IOService *fb){
 auto *connection=s->copyParentEntry(gIOServicePlane);
 bool sameBinding=c->frame==fb && c->connection==connection && endpointEDIDMatches(c);
 if(connection)connection->release();
 OSObject *range=nullptr;
 auto provider=brightnessProvider(s,&range);
 bool sameRange=range && range==c->boundRange;
 if(range)range->release();
 bool sameInterface=true,usableInterface=false;
 if(modeRefreshRequested || c->requests.stopped || (c->initialized && sameBinding && sameRange)){
  unsigned count=0;
  auto *selected=copyUniqueActiveInterface(fb,count);
  auto *types=copyTransactionTypes(selected);
  usableInterface=dbhw::interfaceUsable(count,selected!=nullptr,selected && selected->isInactive(),supportsSimple(types));
  // Absent/ambiguous/unsupported resources do not renew an uninitialized probe.
  sameInterface=c->interface ? (usableInterface && selected==c->interface) : !usableInterface;
  if(types)types->release();if(selected)selected->release();
 }
 return dbhw::endpointRefresh(c->initialized,c->requests.stopped,sameBinding,
  sameInterface,sameRange,provider==dbhw::BrightnessProvider::Foreign,
  modeRefreshRequested,c->requests.busy && !c->requests.stopped,usableInterface);
}
bool queueHardware(IOService *s,IOService *fb,UInt32 desired,bool bootstrap=false){
 // Called inside the native gate; no bus operations here.
 if(!hardwarePilot(s))return false;
 if(!hardwareRequested || !startIO || !hardwareQueue || desired>100 || (!bootstrap && bootSettings.getOnly))return false;
 bool foreign=bootstrap && brightnessProvider(s)==dbhw::BrightnessProvider::Foreign;
 IOLockLock(hardwareQueue);
 bool recovery=bootstrap && modeRefreshRequested && pendingBootstrap.recoveryPending(s,fb);
 bool recoveryTicketFull=false;
 HardwareSlot *slot=nullptr,*empty=nullptr;
 const char *reason="slots-full";unsigned interfaceCount=0;bool retireNeeded=false,retirementPending=false,matchingRetiring=false;
 for(auto &candidate:hardwareSlots){
  unsigned index=static_cast<unsigned>(&candidate-hardwareSlots);
  if(retireCall && dbhw::canRetire(candidate.display!=nullptr,candidate.display && candidate.display->isInactive(),
     candidate.requests.busy,candidate.retiring,busAdmission.parked(index))){
   candidate.retiring=true;retireNeeded=true;
  }
  if(candidate.display==s){
   if(!candidate.retiring && bootstrap && retireCall){
    auto refresh=dbhw::Refresh::Keep;
    if(foreign)refresh=dbhw::Refresh::Retire;
    else if(admissionCall && (modeRefreshRequested || candidate.initialized || candidate.requests.stopped))
     refresh=refreshEndpoint(&candidate,s,fb);
    if(refresh==dbhw::Refresh::Reprobe && !rememberBootstrap(s,fb,modeRefreshRequested)){
     recoveryTicketFull=true;
    }else if(refresh!=dbhw::Refresh::Keep){
     candidate.retiring=true;retireNeeded=true;
     candidate.recoveryReadmit=refresh==dbhw::Refresh::Reprobe;
     if(!candidate.recoveryReadmit)foreign=true;
     const char *decision=candidate.recoveryReadmit?"retire-and-readmit":"native-provider-preserved";
     s->setProperty("DisplayBrightnessEndpointRefresh",decision);
     if(bootSettings.hdrTrace)
      IOLog("DisplayBrightness: mode recovery display=%llu decision=%s\n",s->getRegistryEntryID(),decision);
    }
   }
   if(candidate.retiring){
    matchingRetiring=true;
    if(bootstrap && modeRefreshRequested && candidate.recoveryReadmit)recovery=true;
   }else slot=&candidate;
  }
  retirementPending=retirementPending||dbhw::retirementExpected(candidate.display!=nullptr,
   candidate.display && candidate.display->isInactive(),candidate.retiring);
  if(index<bootSettings.slots && !candidate.retiring && !candidate.display && !empty)empty=&candidate;
 }
 if(foreign || recoveryTicketFull){
  auto ticket=foreign?pendingBootstrap.takePair(s,fb):PendingBootstrap::Ticket{};
  IOLockUnlock(hardwareQueue);
  releaseBootstrapTicket(ticket);
  if(retireNeeded)thread_call_enter(retireCall);
  s->setProperty("DisplayBrightnessBootstrapState",foreign?"native-provider-preserved":"recovery-pending-full");
  return false;
 }
 if(!slot && empty && !matchingRetiring){
  unsigned count=0;bool enumerated=false;
  auto *iface=copyUniqueActiveInterface(fb,count,&enumerated);
  interfaceCount=count;
  reason=enumerated?(count==0?"interface-not-ready":(count!=1?"ambiguous-interface":"missing-transaction-types")):"interface-iterator-unavailable";
  auto *types=copyTransactionTypes(iface);
  bool supported=dbhw::interfaceUsable(count,iface!=nullptr,iface && iface->isInactive(),supportsSimple(types));
  if(types && count==1)reason=supported?"thread-call-allocation-failed":"unsupported-transaction";
  if(types)types->release();
  if(supported || bootstrap){
   if(!empty->call)empty->call=thread_call_allocate_with_priority(hardwareWorker,empty,THREAD_CALL_PRIORITY_LOW);
   if(empty->call && captureEndpointEDID(s,empty->boundEDID)){
    auto *connection=s->copyParentEntry(gIOServicePlane);
    auto *parent=connection?connection->copyParentEntry(gIOServicePlane):nullptr;
    if(hasClass(connection,"IODisplayConnect") && parent==fb){
     empty->recoveryProbe=recovery;
     empty->bootstrapWindow.configure(bootSettings.probeWindow(recovery),bootSettings.startupIntervalMs);
     empty->connection=connection;connection=nullptr;
     empty->display=s;empty->frame=fb;empty->interface=supported?iface:nullptr;
     empty->updateGuard.begin();empty->updateGuard.end(hardwareNowNs());
     for(auto *update=displayUpdates;update;update=update->next)
      if(update->display==s)empty->updateGuard.begin();
     guardState(empty,hardwareNowNs());
     s->retain();fb->retain();slot=empty;if(supported)iface=nullptr;
    }
    if(parent)parent->release();if(connection)connection->release();
   }
  }
  if(iface)iface->release();
 }
 if(slot)reason=slot->frame!=fb?"frame-mismatch":(slot->requests.stopped?"endpoint-invalid-stopped":(slot->requests.failed?"recovering-latest-target":"queued"));
 if(slot && bootstrap && (slot->initialized || slot->requests.busy)){
  auto ticket=slot->frame==fb && (!recovery || slot->recoveryProbe)?
   pendingBootstrap.takePair(s,fb):PendingBootstrap::Ticket{};
  IOLockUnlock(hardwareQueue);
  releaseBootstrapTicket(ticket);
  if(retireNeeded)thread_call_enter(retireCall);
  return true;
 }
 bool schedule=false;
 bool ok=slot && (bootstrap || slot->initialized) && slot->frame==fb && slot->requests.submit(desired,schedule);
 if(ok){
  slot->sequence=nextEvent();
  // Nonblocking cancel only: never wait on a callback from either lock.
  if(!schedule && !bootstrap)
   schedule=slot->requests.advanceRetry([&](){return thread_call_cancel(slot->call);});
  if(schedule){
   bool recoveryStart=bootstrap && slot->recoveryProbe;
   unsigned delay=bootSettings.initialProbeDelay(recoveryStart);
   uint64_t deadline;clock_interval_to_deadline(delay,kMillisecondScale,&deadline);
   if(recoveryStart && bootSettings.hdrTrace)
    IOLog("DisplayBrightness: mode recovery queued display=%llu delayMs=%u windowSeconds=%u\n",
     s->getRegistryEntryID(),delay,bootSettings.probeWindow(true));
   thread_call_enter_delayed(slot->call,deadline);
  }
 }
 bool retryAdmission=false;
 // A full set of active slots must not create an endless admission poll.
 if(!slot && bootstrap && admissionCall && retirementPending){
  retryAdmission=rememberBootstrap(s,fb,recovery);
  reason=retryAdmission?"bootstrap-admission-queued":"bootstrap-pending-full";
 }
 if(!slot && retireNeeded && !bootstrap)reason="retirement-queued-retry-request";
 endpointSnapshot(s,reason,interfaceCount);
 if(!ok)s->setProperty("DisplayBrightnessEndpointFailure",reason);
 auto ticket=ok && bootstrap?pendingBootstrap.takePair(s,fb):PendingBootstrap::Ticket{};
 IOLockUnlock(hardwareQueue);
 releaseBootstrapTicket(ticket);
 if(retireNeeded)thread_call_enter(retireCall);
 // retireWorker owns the admission wakeup; no timer poll while cancellation waits.
 if(!slot)s->setProperty("DisplayBrightnessHardwareState","no-unambiguous-endpoint-or-slot");
 return ok;
}
