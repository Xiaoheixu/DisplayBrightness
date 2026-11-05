# DisplayBrightness

[中文](#中文使用说明) · [English](#english-guide) · [Test release / 测试版下载](https://github.com/Xiaoheixu/DisplayBrightness/releases)

## 中文使用说明

DisplayBrightness 是 macOS 外接显示器原生亮度控制的实验性 Lilu 插件，通过实时 DDC/CI VCP 0x10 能力检测连接系统亮度入口，无显示器型号白名单，不依赖常驻 App 或用户态服务脚本。

首个测试版为 **v0.0.28-debug.1**，kext 的 CFBundle 版本为 **0.0.28**。此版本为测试预发布。

### 依赖与兼容范围

- **x86_64** macOS 内核扩展，依赖 **Lilu 1.7.3 或更高版本**，必须先加载 Lilu。
- 当前代码仅接受 **Darwin 25.6 及指定 IOGraphics UUID**，不是支持全部 macOS 版本的通用驱动。ABI 不匹配时会跳过相关功能。
- 目前只有 IOFramebuffer I2C 内核通信后端。显示器需要支持 DDC/CI 亮度控制，GPU 驱动和连接链路也必须提供可用的通信路径。若显示器菜单提供 DDC/CI 开关，请启用它。
- 当前兼容范围有限；其他设备、GPU、转接链路、睡眠唤醒、热插拔及多屏不保证可用。

### 安装

1. 从 [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) 下载 `DisplayBrightness-0.0.28-debug.1.zip` 并解压，得到 `DisplayBrightness.kext`。ZIP 只包含 kext 包；源码直接保存在本仓库。
2. 备份自己的 EFI、`config.plist` 和旧 kext，并保留可启动的恢复入口。
3. 将 `DisplayBrightness.kext` 放入 `EFI/OC/Kexts/`。确保同目录已有满足版本要求的 `Lilu.kext`。
4. 使用 OpenCore 配置编辑器在 `Kernel -> Add` 中添加并启用本插件，放在 Lilu 条目之后。关键字段如下；已有本插件条目时仅替换文件，避免重复添加。

   | 字段 | 值 |
   | --- | --- |
   | BundlePath | `DisplayBrightness.kext` |
   | ExecutablePath | `Contents/MacOS/DisplayBrightness` |
   | PlistPath | `Contents/Info.plist` |
   | Enabled | `True` |

5. 保存配置并重启。正常启用无需本插件专用启动参数；如果已有 `-dboff`，需要移除它才能启用本插件。

### 使用与验证

重启后分别测试 **系统设置 → 显示器**、**控制中心 → 显示器**以及键盘亮度键，并观察显示器实际亮度与滑块是否同步。插件自动检测能力，无额外 App 需要启动。多屏尚未完成实测，不保证亮度键能控制指定外接屏。

UI 的 0–100 为归一化百分比，实际值根据显示器实时返回的 maximum 换算；0 不代表显示器物理背光一定关闭。EDID 合法只允许探测，只有有效 GetVCP 回复才允许亮度写入。

若没有亮度入口或硬件没有变化，请先核对 Lilu/插件条目和加载顺序、`-dboff`、Darwin/IOGraphics ABI 条件、显示器 DDC/CI 及连接链路。该插件不能通过放宽设备校验解决不支持的通信路径。

### 禁用与回退

- 临时禁用：在自己的 OpenCore 启动参数中加入 `-dboff`，保存后重启。
- 停用插件：将对应 `Kernel -> Add` 条目的 `Enabled` 改为 `False`，保存后重启。
- 回退：恢复自己备份的旧 kext 和配置；必要时使用预先保留的恢复入口。不要直接删除其他驱动或替换整套 EFI。

## English guide

DisplayBrightness is an experimental Lilu plugin for native macOS external-display brightness controls. It probes live DDC/CI VCP 0x10 capabilities without a monitor-model allowlist. No persistent app or user-space service script is required.

The first test release is **v0.0.28-debug.1**; the kext CFBundle version is **0.0.28**. This is a test prerelease.

### Requirements and compatibility

- **x86_64** macOS kernel extension; requires **Lilu 1.7.3 or later**, loaded before this plugin.
- The current code accepts only **Darwin 25.6 and a specific IOGraphics UUID**. Other OS builds are not guaranteed to work; incompatible ABI paths are skipped.
- Only the IOFramebuffer I2C kernel backend is implemented. The monitor must support DDC/CI brightness control, and the GPU driver and connection path must expose working communication. Enable DDC/CI in the monitor menu if that option exists.
- Compatibility is limited. Other hardware, GPUs, adapters, sleep/wake, hot-plug and multiple monitors are not guaranteed to work.

### Installation

1. Download `DisplayBrightness-0.0.28-debug.1.zip` from [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) and extract `DisplayBrightness.kext`. The ZIP contains only the kext bundle; source code is committed directly to this repository.
2. Back up your EFI, `config.plist` and previous kext, and keep a bootable recovery option.
3. Copy `DisplayBrightness.kext` to `EFI/OC/Kexts/`. Ensure a compatible `Lilu.kext` is present.
4. In your OpenCore configuration editor, add and enable the plugin under `Kernel -> Add`, after Lilu. Use the values below. If an entry already exists, replace the kext without adding a duplicate entry.

   | Field | Value |
   | --- | --- |
   | BundlePath | `DisplayBrightness.kext` |
   | ExecutablePath | `Contents/MacOS/DisplayBrightness` |
   | PlistPath | `Contents/Info.plist` |
   | Enabled | `True` |

5. Save the configuration and reboot. No plugin-specific boot argument is needed for normal operation. Remove `-dboff` if it is already present and you want to enable the plugin.

### Usage and verification

After rebooting, test **System Settings → Displays**, **Control Center → Display**, and the keyboard brightness keys. Check that physical monitor brightness changes and the slider stays synchronized. Capability detection is automatic; no extra app needs to be launched. Multi-monitor behavior remains unverified, including which external monitor receives keyboard brightness changes.

The UI range of 0–100 is a normalized percentage calculated from the monitor's live maximum; zero does not necessarily turn the physical backlight off. Valid EDID permits probing only. Brightness writes require a valid live GetVCP reply.

If controls are missing or brightness does not change, check the Lilu/plugin entries and load order, `-dboff`, Darwin/IOGraphics ABI requirements, monitor DDC/CI settings and connection path. Relaxing device validation cannot supply a missing communication backend.

### Disable and roll back

- Temporarily disable the plugin: add `-dboff` to your OpenCore boot arguments, save and reboot.
- Disable its `Kernel -> Add` entry by setting `Enabled` to `False`, then save and reboot.
- To roll back, restore your own previous kext and configuration, using your recovery boot option if needed. Do not remove unrelated drivers or replace your entire EFI with another machine's configuration.

## Issues

遇到问题请提交 [GitHub Issue](https://github.com/Xiaoheixu/DisplayBrightness/issues)。 / Please report problems through [GitHub Issues](https://github.com/Xiaoheixu/DisplayBrightness/issues).
