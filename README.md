# DisplayBrightness

[简体中文](README.zh-CN.md)

DisplayBrightness is an OpenCore-loaded Lilu plugin that adds native macOS brightness control to compatible external monitors. Use **System Settings → Displays**, **Control Center**, or the system brightness keys to adjust the monitor's hardware brightness through DDC/CI. Normal operation requires only the kext and Lilu.

## How it works

1. The plugin checks the Darwin version and x86_64 IOGraphics UUID before installing its display parameter hooks.
2. It associates each eligible external display with its framebuffer and an unambiguous I²C interface. A bounded DDC/CI GetVCP probe verifies support for brightness (VCP `0x10`) and reads the monitor's current and maximum values.
3. After a valid reply, the plugin publishes brightness controls for that display. Requests are coalesced and handled by a serialized hardware worker. The worker reads the brightness before writing, scales the requested percentage to the monitor's range, and verifies the result by reading the hardware back.
4. Displays with an existing native brightness provider stay under that provider's control. The plugin rechecks endpoint identity and ownership before hardware I/O.

On an interface that advertises the DDC/CI reply transaction type, the startup probe can try that reply mode after a failed ordinary I²C read. The mode that succeeds is retained for the initialized endpoint. A value of 0% maps to the monitor's lowest requested hardware value, which can still emit light.

## Requirements

- x86_64 macOS with a matching IOGraphics profile, OpenCore, and [Lilu 1.7.3 or later](https://github.com/acidanthera/Lilu/releases).
- An external monitor with working DDC/CI brightness control and a framebuffer I²C path exposed by its GPU driver and connection. Docks, adapters, and display modes can affect the available path.
- DDC/CI enabled in the monitor's on-screen menu, if the monitor offers that setting.

| Reference macOS build | Darwin | Enablement |
| --- | --- | --- |
| Ventura 13.7.6 (`22H625`) | 22.6 | `-dbcompat` |
| Sonoma 14.8.9 (`23J631`) | 23.6 | `-dbcompat` |
| Sequoia 15.8 (`24H23`), 15.8.2 (`24H232`) | 24.6 | `-dbcompat` |
| Tahoe 26.7.2 (`25G332`) | 25.6 | Automatic |

The build numbers identify reference profiles. The driver matches the Darwin major/minor version and IOGraphics UUID at runtime.

## Installation

1. Download the **RELEASE** ZIP from [GitHub Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases). Use the **DEBUG** ZIP for diagnostics or crash symbolication.
2. Back up your EFI and keep a working OpenCore boot option. Extract the ZIP and copy `DisplayBrightness.kext` into `EFI/OC/Kexts`.
3. Enable the kext under `Kernel → Add` after Lilu. Use `Contents/Info.plist` for `PlistPath` and `Contents/MacOS/DisplayBrightness` for `ExecutablePath`. Install one build variant at a time.
4. Append any applicable arguments to the existing OpenCore `boot-args` under `NVRAM → Add → 7C436110-AB2A-4BBB-A880-FE41995C9F82`. Save and restart.
5. Select the external monitor in **System Settings → Displays** or use its slider in **Control Center**. To roll back, disable the kext's `Kernel → Add` entry or restore the previous EFI configuration.

## Boot arguments

Switches are enabled by their presence and off by default. Remove the entire switch to disable it. Numeric options use decimal `name=value` syntax. Out-of-range values leave the default in effect.

### General and diagnostic options

| Argument | Effect |
| --- | --- |
| `-dboff` | Disable the plugin through Lilu. |
| `-dbcompat` | Enable matching Ventura, Sonoma, and Sequoia profiles. The IOGraphics UUID check still applies. |
| `-dbbootlog` | Record startup checkpoints and DDC probe results for manual TXT export. Enables additional Lilu plugin startup logging in the Debug build. |
| `-dbnoddc` | Install hooks while skipping this plugin's DDC operations and brightness publication. |
| `-dbgetonly` | Probe GetVCP in read-only mode, without publishing brightness controls or sending SetVCP writes. |
| `-dbtrace` | Log display updates, brightness requests, and probes. |
| `-dbnoedm` | Skip the plugin's EDM capability marker for comparison. |
| `-dbnonotify` | Skip brightness-change notifications while updating verified values. |

`-dbnoddc` takes precedence over `-dbgetonly`. Both modes skip experimental endpoint recovery.

### DDC tuning

| Argument | Effect | Default | Range |
| --- | --- | --- | --- |
| `dbslots=N` | Endpoint slot limit | 8 | 1–8 |
| `dbcoalesce=N` | Delay for combining rapid brightness requests | 50 ms | 50–500 ms |
| `dbsettle=N` | Delay between SetVCP and readback | 50 ms | 50–500 ms |
| `dbreply=N` | Minimum I²C reply delay | 40 ms | 40–200 ms |
| `dbretries=N` | Automatic retry limit after a failed request on an initialized endpoint | 2 | 0–2 |
| `dbbootwindow=N` | Startup probe window | 60 s | 5–60 s |
| `dbbootinterval=N` | Delay before retrying a failed startup probe | 1,000 ms | 1,000–5,000 ms |

### HDR and mode changes

| Argument | Effect | Default | Range |
| --- | --- | --- | --- |
| `-dbhdr` | Enable experimental endpoint recovery after eligible display-mode changes | Off | — |
| `-dbhdrtrace` | Log endpoint retirement, re-admission, and recovery probes | Off | — |
| `dbhdrdelay=N` | Extra delay before the first recovery probe | 0 ms | 0–3,000 ms |
| `dbhdrwindow=N` | Recovery probe window cap, also limited by `dbbootwindow` | 60 s | 5–60 s |

The three recovery options require `-dbhdr`. This feature re-probes brightness after a display-mode change. Hardware brightness control still requires the monitor to accept DDC/CI in the active mode.

## What to include in a report

Open an [issue](https://github.com/Xiaoheixu/DisplayBrightness/issues) with the following information.

- macOS version and build from `sw_vers`, OpenCore and Lilu versions, and DisplayBrightness version and build variant.
- GPU model, monitor model, port, cable, and any dock, adapter, or KVM in the signal path. For a laptop, say whether the internal display is active.
- HDR state, the steps to reproduce, and whether the native controls respond and the monitor's actual brightness changes.
- DisplayBrightness boot arguments and a `DisplayBrightness-boot.txt` report, when available. Add `-dbbootlog`, reproduce after restarting, then run `ExportBootLog.command` from the release ZIP. The script collects recent driver logs and the current framebuffer topology.
- For a failed boot, collect the report from a working boot and include the failed attempt's OpenCore log if available. Pass it to the exporter with `--opencore "/path/to/opencore-log.txt"`. For a panic or freeze, include the corresponding report and approximate time.

Review the report before sharing it and remove personal details, serial numbers, and any sensitive configuration. Keep the Debug build's matching `.dSYM` for symbolication.

## Credits

- [Acidanthera and the Lilu contributors](https://github.com/acidanthera/Lilu/graphs/contributors) for Lilu.
- [OpenCorePkg](https://github.com/acidanthera/OpenCorePkg) and [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK) for boot integration and kernel headers.
- [Apple's IOGraphics sources](https://github.com/apple-oss-distributions/IOGraphics) and [MonitorControl](https://github.com/MonitorControl/MonitorControl) as references for display plumbing and DDC/CI behavior.
- Community testers for checking real hardware and sharing display configurations.
