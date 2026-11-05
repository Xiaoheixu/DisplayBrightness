DisplayBrightness
=================

[简体中文](README.zh-CN.md)

A Lilu plugin providing native macOS brightness control for compatible external monitors through DDC/CI.

#### Features

- Native brightness controls in Control Center, System Settings, and keyboard brightness keys.
- Live DDC/CI VCP 0x10 capability detection without a monitor-model allowlist.
- Brightness scaling based on the monitor's reported maximum, with hardware readback and slider synchronization.
- Automatic startup without a companion application or background service.

#### Installation

Install `DisplayBrightness.kext` together with [Lilu](https://github.com/acidanthera/Lilu) **1.7.3 or later**.

Place the kext in `EFI/OC/Kexts` and enable it under OpenCore `Kernel -> Add`, after Lilu. Save the configuration and reboot. Back up your EFI and existing configuration before making changes.

Prebuilt binaries are distributed through [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases).

#### Configuration

- No plugin-specific boot argument is required for normal operation.
- Add `-dboff` to disable the plugin. Changes take effect after rebooting.
- Enable DDC/CI in the monitor's settings if that option is available.

After rebooting, use the native brightness controls to adjust the monitor. To roll back, disable the plugin's `Kernel -> Add` entry or restore your previous kext and configuration, then reboot.

#### Peculiarities

- x86_64 only. Currently restricted to **Darwin 25.6 and a specific IOGraphics UUID** for ABI compatibility.
- Requires a working **IOFramebuffer I2C** communication path. DDC/CI support on the monitor alone does not guarantee compatibility with the GPU driver, cable, or adapter.
- Other hardware, sleep/wake, hot-plug, and multi-monitor configurations remain unverified.
- The system slider uses a normalized percentage. A value of zero does not necessarily turn off the physical backlight.

This is a test prerelease. Please report problems through [Issues](https://github.com/Xiaoheixu/DisplayBrightness/issues).

#### Credits

- [Acidanthera](https://github.com/acidanthera) and the [Lilu contributors](https://github.com/acidanthera/Lilu/graphs/contributors) for Lilu.
- [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK) for kernel headers and libraries.
