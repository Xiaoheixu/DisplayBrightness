from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
fixture = '''+-o AMDFramebuffer <class AMDFramebuffer, id 0x123, active>
  | "DisplayBrightnessDDCEnabled" = Yes
  | "DisplayBrightnessHardwareDesired" = 75
  | "DisplayBrightnessHardwareTarget" = 150
  | "DisplayBrightnessHardwareVerified" = 149
  | "DisplayBrightnessIntegerExit" = {"stage"="hardware-queued"}
  | "DisplayBrightnessBootstrapHistory" = (
  |   {"round"=1,"reply"=<6e88020010000064005130>},
  |   {"round"=2,"replyValid"=Yes}
  | )
  | "DisplayBrightnessEndpointSnapshot" = {
  |   "slot0" = {
  |     "pending" = Yes
  |   }
  | }
  | "DisplayBrightnessDDCGuardState" = "settling"
  | "IODisplayEDID" = <private-serial-number>
  | "UnrelatedSecret" = "do-not-export"
  | "UnrelatedObject" = {"DisplayBrightnessHardwareState" = "secret-nested-value"}
'''
r = subprocess.run(['bash', str(root/'ExportBootLog.command'), '--filter-ioreg'],
                   input=fixture, text=True, capture_output=True)
assert r.returncode == 0, r.stderr
for value in ['AMDFramebuffer', 'DDCEnabled', 'HardwareDesired', 'HardwareTarget',
              'HardwareVerified', 'hardware-queued', '6e88020010000064005130',
              'replyValid', 'EndpointSnapshot', '"pending" = Yes', 'DDCGuardState']:
    assert value in r.stdout, (value, r.stdout)
assert 'private-serial-number' not in r.stdout
assert 'UnrelatedSecret' not in r.stdout
assert 'secret-nested-value' not in r.stdout
# Malformed/oversized collections must not turn a diagnostic filter into a dump.
huge = '"DisplayBrightnessBootstrapHistory" = (\n' + 'x'*200 + '\n'*200 + '"UnrelatedSecret" = "secret"\n'
r = subprocess.run(['bash', str(root/'ExportBootLog.command'), '--filter-ioreg'],
                   input=huge, text=True, capture_output=True, check=True)
assert len(r.stdout) < 33000
assert '"UnrelatedSecret"' not in r.stdout
print('diagnostic export: nested replies, request path, guard state, redaction and bounds pass')
