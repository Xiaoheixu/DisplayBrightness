# DisplayBrightness

[English](README.md)

通过 DDC/CI 为兼容的外接显示器提供 macOS 原生亮度控制的 Lilu 插件。

#### 功能

- 支持系统设置、控制中心及键盘亮度键等原生入口。
- 根据实时 VCP 0x10 亮度能力检测设备，不使用显示器型号白名单。
- 按显示器返回的最大值换算亮度，并同步硬件亮度与系统滑块。
- 无需配套 App 或后台服务。

#### 兼容条件

- x86_64 macOS，依赖 [Lilu](https://github.com/acidanthera/Lilu) 1.7.3 或更高版本。
- 当前受 **Darwin 25.6 及指定 IOGraphics UUID** 限制，不会自动适配其他 macOS 构建。
- 显示器需支持 DDC/CI 亮度控制，GPU 驱动及连接链路需提供可用的 IOFramebuffer I2C 通信路径。如果显示器菜单提供 DDC/CI 开关，请启用它。
- 不保证其他硬件、转接链路、睡眠唤醒、热插拔及多屏可用。

#### 安装

1. 从 [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) 下载 kext ZIP，解压得到 `DisplayBrightness.kext`。
2. 备份自己的 EFI、`config.plist` 和旧 kext，并保留可启动的恢复入口。
3. 将 kext 放入 `EFI/OC/Kexts/`，确保同目录已有满足版本要求的 `Lilu.kext`。
4. 在 OpenCore `Kernel -> Add` 中添加并启用以下条目，放在 **Lilu 之后**。已有条目时只替换 kext，避免重复添加。

| 字段 | 值 |
| --- | --- |
| BundlePath | `DisplayBrightness.kext` |
| ExecutablePath | `Contents/MacOS/DisplayBrightness` |
| PlistPath | `Contents/Info.plist` |
| Enabled | `True` |

5. 保存配置并重启。

#### 使用

打开 **系统设置 → 显示器**、**控制中心 → 显示器**，或使用键盘亮度键。确认显示器实际亮度发生变化，且滑块保持同步，无需另外启动 App。

UI 的 0–100 是根据显示器返回最大值换算的百分比，0 不代表物理背光一定关闭。多屏情况下，不保证亮度键能控制指定外接屏。

#### 配置

- 正常启用无需本插件专用启动参数。
- 在 OpenCore 启动参数中加入 `-dboff`，保存并重启即可禁用；移除此参数后可重新启用。
- 也可禁用对应的 `Kernel -> Add` 条目。需要回退时恢复自己备份的旧 kext 和配置，再重启；必要时使用恢复入口。

如果没有亮度入口，请检查 Lilu 与插件条目、加载顺序、`-dboff`、系统 ABI 条件、显示器 DDC/CI 设置及连接链路。

遇到问题请提交 [Issue](https://github.com/Xiaoheixu/DisplayBrightness/issues)。
