# DisplayBrightness v0.0.28-debug.1

## 中文

首个测试预发布。通过 DDC/CI 为外接显示器提供 macOS 原生亮度控制入口，正常自动启用，`-dboff` 可禁用。

仅支持 x86_64，依赖 Lilu 1.7.3+，当前受 Darwin 25.6 / 指定 IOGraphics UUID 限制，显示器及连接链路须支持可用的 DDC/CI 通信。不保证其他硬件、睡眠唤醒、热插拔和多屏可用。

下载并解压 ZIP，将 `DisplayBrightness.kext` 放入 `EFI/OC/Kexts`，在 OpenCore `Kernel -> Add` 中启用并放在 Lilu 之后，保存并重启。操作前请备份自己的 EFI。完整安装、使用及回退步骤见 [README.md](https://github.com/Xiaoheixu/DisplayBrightness/blob/main/README.md)。

遇到问题请提交 [Issue](https://github.com/Xiaoheixu/DisplayBrightness/issues)。

## English

First test prerelease. Provides native macOS brightness controls for external monitors through DDC/CI. Enabled automatically; use `-dboff` to disable.

x86_64 only; requires Lilu 1.7.3+. Currently limited to Darwin 25.6 and a specific IOGraphics UUID. The monitor and connection path must provide working DDC/CI communication. Other hardware, sleep/wake, hot-plug and multiple monitors are not guaranteed to work.

Extract the ZIP, copy `DisplayBrightness.kext` to `EFI/OC/Kexts`, enable it in OpenCore `Kernel -> Add` after Lilu, save and reboot. Back up your own EFI first. See [README.md](https://github.com/Xiaoheixu/DisplayBrightness/blob/main/README.md) for complete installation, usage and rollback instructions.

Please report problems through [Issues](https://github.com/Xiaoheixu/DisplayBrightness/issues).
