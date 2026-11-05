DisplayBrightness
=================

[English](README.md)

通过 DDC/CI 为兼容的外接显示器提供 macOS 原生亮度控制的 Lilu 插件。

#### 功能

- 支持控制中心、系统设置及键盘亮度键等原生入口。
- 实时检测 DDC/CI VCP 0x10 亮度能力，无显示器型号白名单。
- 按显示器返回的最大值换算亮度，支持硬件读回与滑块状态同步。
- 随系统自动启用，无需配套应用或后台服务。

#### 安装

本插件须与 [Lilu](https://github.com/acidanthera/Lilu) **1.7.3 或更高版本**配合使用。

将 `DisplayBrightness.kext` 放入 `EFI/OC/Kexts`，在 OpenCore `Kernel -> Add` 中启用，并放在 Lilu 之后。保存配置并重启。操作前请备份自己的 EFI 和现有配置。

预编译版本通过 [Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) 提供。

#### 配置

- 正常运行无需本插件专用启动参数。
- 添加 `-dboff` 可禁用本插件，修改后重启生效。
- 如果显示器菜单提供 DDC/CI 开关，请将其启用。

重启后即可通过原生亮度入口调节兼容的显示器。需要回退时，禁用对应的 `Kernel -> Add` 条目，或恢复自己的旧 kext 与配置，再重启。

#### 注意事项

- 仅支持 x86_64，目前受 **Darwin 25.6 及指定 IOGraphics UUID** 的 ABI 兼容性条件限制。
- 需要可用的 **IOFramebuffer I2C** 通信路径。显示器支持 DDC/CI 不代表显卡驱动、线缆或转接设备一定兼容。
- 其他硬件、睡眠唤醒、热插拔及多屏配置仍待验证。
- 系统滑块显示归一化百分比，数值为 0 不代表物理背光一定关闭。

本版本为测试预发布。遇到问题请提交 [Issue](https://github.com/Xiaoheixu/DisplayBrightness/issues)。

#### 致谢

- [Acidanthera](https://github.com/acidanthera) 及 [Lilu 贡献者](https://github.com/acidanthera/Lilu/graphs/contributors)提供 Lilu。
- [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK) 提供内核头文件及库。
