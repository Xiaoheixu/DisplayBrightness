# Export only display topology and selected plugin diagnostics. Collections are
# bounded independently; never dump arbitrary EDID, parameters or registry data.
function balance(line,    i,c,quoted,escaped,n) {
 quoted=0;escaped=0;n=0
 for(i=1;i<=length(line);i++) {
  c=substr(line,i,1)
  if(escaped){escaped=0;continue}
  if(quoted && c=="\\"){escaped=1;continue}
  if(c=="\""){quoted=!quoted;continue}
  if(!quoted){if(c=="(" || c=="{")n++;if(c==")" || c=="}")n--}
 }
 return n
}
/^\+-o |\+-o .*<class ([^,]*:)?(IOFramebufferI2CInterface|IODisplayConnect|AppleBacklightDisplay|AppleDisplay)(:[^,]*)?,/ {depth=0;print;next}
/^[ |]*"(IOFBI2CInterfaceIDs|IOI2CInterfaceID|IOI2CBusType|IOI2CTransactionTypes|DisplayBrightness(RequestProbeRouting|RequestProbeVersion|GraphicsProfile|StartupMode|HDRRecoveryEnabled|DDCEnabled|Tuning|I2CActiveCount|EDMProbeState|EndpointFailure|BootstrapState|BootstrapAttemptCount|BootstrapHistory|HardwareState|HardwareOperation|HardwareReplyType|HardwareReply|HardwareActual|HardwareMaximum|HardwareDesired|HardwareTarget|HardwareVerified|HardwareTransportResult|HardwareRequestResult|EndpointRefresh|EndpointSnapshot|IntegerEntry|IntegerExit|BrightnessEntry|BrightnessExit|DDCGuardState|DDCGuardGeneration|DDCGuardUntilUptimeNs|DDCDeferredCycles))" =/ {
 propertyColumn=index($0,"\"");depth=balance($0);lines=0;bytes=length($0)
 if(bytes<=32768)print;else {print "[diagnostic property truncated]";depth=0}
 next
}
depth>0 {
 # A new registry property cannot be part of a malformed prior collection.
 if($0 ~ /^[ |]*"[^"]+" =/ && index($0,"\"")<=propertyColumn){depth=0;next}
 lines++;bytes+=length($0)
 if(lines>128 || bytes>32768){print "[diagnostic collection truncated]";depth=0;next}
 print;depth+=balance($0)
}
