# DisplayBrightness

macOS 外接显示器原生亮度控制实验性 Lilu 插件。

首个 GitHub 测试发布：**v0.0.28-debug.1**，驱动内部版本仍为 **0.0.28**。Debug 表示开发测试预发布，沿用当前已验证构建参数。

通过显示器实时 DDC/CI VCP 0x10 能力检测，连接原生亮度入口；不使用显示器型号白名单，不依赖常驻 App 或用户态服务脚本。正常自动启用，`-dboff` 可禁用插件。

## 兼容范围

- x86_64 macOS 内核扩展，依赖 Lilu 1.7.3 或更高版本。
- 当前代码受 Darwin 25.6 和指定 IOGraphics UUID 的 ABI 门槛限制，不保证其他系统构建可用。
- 目前仅实现 IOFramebuffer I2C 内核通信后端；显示器、GPU 驱动及连接链路必须支持实际 DDC 通信。
- 当前 ANT321FC 单屏已有一次冷启动以及控制中心、系统设置、亮度键、真实硬件变化和滑块同步验收。其他设备、睡眠唤醒、热插拔及多屏仍待实测。

## 安装与回退

1. 下载 Release 中的 `DisplayBrightness-0.0.28-debug.1.zip` 并解压。
2. 备份自己的 EFI 和 OpenCore 配置，保留可启动的恢复入口。
3. 将 `DisplayBrightness.kext` 放入 `EFI/OC/Kexts`，在 OpenCore `Kernel -> Add` 中启用，并确保 Lilu 先于本插件加载。已有条目时仅替换 kext。
4. 重启后分别验证三个原生入口、实际亮度变化和滑块同步。包内不提供机器专用 EFI 配置。
5. 需要停用时添加 `-dboff` 或禁用本插件的 `Kernel -> Add` 条目；需要回退时恢复自己保存的旧 kext 和配置。

## 构建

需要 macOS、Xcode 命令行工具及 Python 3。构建依赖快照和第三方许可证包含在 `driver/vendor`。

```sh
sh driver/build.sh
```

构建执行 21 项 C++ 测试及源码范围检查，产物为 `driver/build/DisplayBrightness.kext`。构建命令不会安装或加载驱动。

## 文档与反馈

- [驱动说明](driver/README.zh-CN.md)
- [通用能力检测及边界](driver/docs/GENERAL-DETECTION.zh-CN.md)
- [验证记录摘要](driver/docs/VALIDATION.zh-CN.md)
- [首发说明](RELEASE-NOTES.md)

反馈请记录 macOS 构建号、GPU 驱动、连接链路、显示器型号、GetVCP 结果和各原生入口的表现。提交日志前请移除设备序列号及其他个人信息。

本仓库仅包含当前源码、测试、文档及构建依赖；机器配置、运行日志、回退快照及历史实验保留在本地。Lilu 和 MacKernelSDK 的许可证见各自 vendor 目录，二进制包随附 Lilu 许可证。
