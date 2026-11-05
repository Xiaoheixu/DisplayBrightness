# DisplayBrightness

[简体中文](README.zh-CN.md)

A Lilu plugin that brings native macOS brightness controls to compatible external monitors through DDC/CI.

#### Features

- Native controls in System Settings, Control Center and keyboard brightness keys.
- Detects live VCP 0x10 brightness capabilities without a monitor-model allowlist.
- Scales brightness to the monitor's reported maximum and synchronizes hardware brightness with the system slider.
- No companion app or background service required.

#### Requirements

- x86_64 macOS and [Lilu](https://github.com/acidanthera/Lilu) 1.7.3 or later.
- Currently limited to **Darwin 25.6 and a specific IOGraphics UUID**. Other macOS builds are not supported automatically.
- A monitor with DDC/CI brightness support and a working IOFramebuffer I2C path through the GPU driver and connection. Enable DDC/CI in the monitor menu if available.
- Compatibility with other hardware, adapters, sleep/wake, hot-plug and multiple monitors is not guaranteed.

#### Installation

1. Download the kext ZIP from [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) and extract `DisplayBrightness.kext`.
2. Back up your EFI, `config.plist` and previous kext. Keep a bootable recovery option.
3. Copy the kext to `EFI/OC/Kexts/`, alongside a compatible `Lilu.kext`.
4. Add and enable the following entry under OpenCore `Kernel -> Add`, **after Lilu**. If an entry already exists, replace the kext without adding a duplicate.

| Field | Value |
| --- | --- |
| BundlePath | `DisplayBrightness.kext` |
| ExecutablePath | `Contents/MacOS/DisplayBrightness` |
| PlistPath | `Contents/Info.plist` |
| Enabled | `True` |

5. Save the configuration and reboot.

#### Usage

Open **System Settings → Displays** or **Control Center → Display**, or use the keyboard brightness keys. Check that physical monitor brightness changes and the slider stays synchronized. No extra app needs to be opened.

The UI range of 0–100 is a percentage of the monitor's reported maximum. Zero does not necessarily turn the backlight off. Multi-monitor keyboard targeting is not guaranteed.

#### Configuration

- No plugin-specific boot argument is required for normal operation.
- Add `-dboff` to your OpenCore boot arguments to disable the plugin; save and reboot. Remove it to enable the plugin again.
- You can also disable its `Kernel -> Add` entry. To roll back, restore your own previous kext and configuration, then reboot using your recovery option if needed.

If controls are missing, check Lilu and plugin entries, load order, `-dboff`, the supported system ABI, monitor DDC/CI settings and connection path.

Please report problems through [Issues](https://github.com/Xiaoheixu/DisplayBrightness/issues).
