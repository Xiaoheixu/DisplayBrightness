# DisplayBrightness

[English](README.md)

DisplayBrightness 是通过 OpenCore 加载的 Lilu 插件，为兼容的外接显示器提供 macOS 原生亮度控制。你可以用「系统设置 → 显示器」、控制中心或键盘亮度键，通过 DDC/CI 调节显示器的实际硬件亮度。日常使用只需 DisplayBrightness 和 Lilu 两个 kext，无需配套 App 或后台服务。

## 实现原理

1. **检查系统兼容性。** 驱动核对 Darwin 主次版本和 x86_64 IOGraphics 的 UUID，匹配后才安装显示参数挂钩。
2. **探测亮度能力。** 驱动将外接显示器与对应的 framebuffer 和唯一可用的 I²C 接口绑定，在限定时间内发送 DDC/CI GetVCP 请求，读取亮度命令 VCP `0x10` 的当前值和最大值。普通 I²C 读取失败时，若接口声明支持 DDC/CI 专用回复，驱动可以在后续探测轮次尝试该方式。
3. **接入原生亮度控制。** 收到有效回复后，驱动才向系统公开该显示器的亮度能力。驱动合并连续的滑块输入并串行处理硬件请求。每次写入前先读取当前亮度，再根据显示器返回的范围换算百分比，并在写入后读回核对。
4. **保留系统原有的控制。** 已有 macOS 原生亮度控制的显示器继续由系统管理。每次访问硬件前，驱动都会核对显示器、接口和亮度控制归属。

滑块的 0% 对应向显示器请求的最低硬件亮度，屏幕此时仍可能发光。

## 使用要求

- x86_64 macOS、OpenCore 和 [Lilu 1.7.3 或更高版本](https://github.com/acidanthera/Lilu/releases)。
- 显示器支持通过 DDC/CI 调节亮度，且显卡驱动为当前连接提供可用的 framebuffer I²C 通信路径。扩展坞、转接器、KVM 和显示模式都可能影响这条路径。
- 如果显示器的屏幕菜单（OSD）提供 DDC/CI 开关，请先开启。

| macOS 参考构建 | Darwin | 启用方式 |
| --- | --- | --- |
| Ventura 13.7.6（`22H625`） | 22.6 | 添加 `-dbcompat` |
| Sonoma 14.8.9（`23J631`） | 23.6 | 添加 `-dbcompat` |
| Sequoia 15.8（`24H23`）、15.8.2（`24H232`） | 24.6 | 添加 `-dbcompat` |
| Tahoe 26.7.2（`25G332`） | 25.6 | 匹配时自动启用 |

表中的构建号用于标识参考配置。驱动运行时以 Darwin 主次版本和 IOGraphics UUID 的组合为准。

## 安装与回退

1. 从 [GitHub Releases](https://github.com/Xiaoheixu/DisplayBrightness/releases) 下载 **RELEASE** 压缩包。需要额外启动日志或崩溃符号时，使用 **DEBUG** 压缩包。
2. 备份当前 EFI，保留一个可用的 OpenCore 启动选项。解压后，将 `DisplayBrightness.kext` 放入 `EFI/OC/Kexts`。
3. 在 OpenCore 的 `Kernel → Add` 中启用该 kext，并将它排在 Lilu 之后。Release 和 Debug 任选其一。

   | 配置项 | 值 |
   | --- | --- |
   | `BundlePath` | `DisplayBrightness.kext` |
   | `ExecutablePath` | `Contents/MacOS/DisplayBrightness` |
   | `PlistPath` | `Contents/Info.plist` |
   | `Enabled` | `True` |

4. 如需设置启动参数，在 `NVRAM → Add → 7C436110-AB2A-4BBB-A880-FE41995C9F82` 下将参数追加到现有的 `boot-args)，然后保存配置并重启。
5. 进入 macOS 后，在「系统设置 → 显示器」选中外接屏，或打开控制中心使用该屏的亮度滑块。需要回退时，禁用对应的 `Kernel → Add` 条目，或恢复原来的 EFI 配置后重启。

## 启动参数

开关型参数默认关闭，添加即启用，移除整项即关闭。数值参数使用十进制 `名称=值` 格式。超出范围时使用默认值。

### 通用与诊断

| 参数 | 作用 |
| --- | --- |
| `-dboff` | 通过 Lilu 禁用 DisplayBrightness。 |
| `-dbcompat` | 启用匹配的 Ventura、Sonoma 和 Sequoia 配置，仍需通过 IOGraphics UUID 检查。 |
| `-dbbootlog` | 记录启动阶段和初始 DDC 探测结果，供手动导出 TXT。Debug 版还会启用额外的 Lilu 插件启动日志。 |
| `-dbnoddc` | 保留挂钩，跳过本驱动的 DDC 操作和亮度能力发布。 |
| `-dbgetonly` | 仅用 GetVCP 进行有界的只读探测，跳过亮度能力发布、SetVCP 写入和亮度通知。 |
| `-dbtrace` | 记录显示更新、亮度请求和探测的详细日志。 |
| `-dbnoedm` | 跳过本驱动的 EDM 能力标记，用于对照原生滑块的识别行为。 |
| `-dbnonotify` | 继续更新已验证的亮度值，同时跳过亮度变化通知。 |

`-dbnoddc` 优先于 `-dbgetonly`。两种模式均会关闭实验性端点恢复。

### DDC 调整

| 参数 | 作用 | 默认值 | 范围 |
| --- | --- | --- | --- |
| `dbslots=N` | 可分配的显示端点槽位上限 | 8 | 1–8 |
| `dbcoalesce=N` | 合并连续亮度请求的等待时间 | 50 ms | 50–500 ms |
| `dbsettle=N` | SetVCP 写入后到读回前的等待时间 | 50 ms | 50–500 ms |
| `dbreply=N` | I²C 最小回复等待时间 | 40 ms | 40–200 ms |
| `dbretries=N` | 已初始化端点的请求失败后，自动重试的次数上限 | 2 | 0–2 |
| `dbbootwindow=N` | 启动探测的时间窗口 | 60 秒 | 5–60 秒 |
| `dbbootinterval=N` | 启动探测失败后的重试间隔 | 1,000 ms | 1,000–5,000 ms |

### HDR 与显示模式切换

| 参数 | 作用 | 默认值 | 范围 |
| --- | --- | --- | --- |
| `-dbhdr` | 在符合条件的显示模式变化后启用实验性端点恢复 | 关 | — |
| `-dbhdrtrace` | 记录旧端点回收、重新绑定和恢复探测过程 | 关 | — |
| `dbhdrdelay=N` | 首次恢复探测前额外等待的时间 | 0 ms | 0–3,000 ms |
| `dbhdrwindow=N` | 恢复探测时间窗口上限，同时受 `dbbootwindow` 限制 | 60 秒 | 5–60 秒 |

后三项需配合 `-dbhdr` 使用。该功能会在模式变化后重新探测亮度。HDR 下能否调节硬件亮度，仍取决于显示器在当前模式下是否接受 DDC/CI 亮度命令。

## 反馈时请提供

请在 [GitHub Issues](https://github.com/Xiaoheixu/DisplayBrightness/issues) 提交以下信息。

- `sw_vers` 输出，以及 OpenCore、Lilu、DisplayBrightness 的版本，注明使用 Release 还是 Debug 包。
- 显卡和显示器的完整型号、连接端口及线材，注明是否经过扩展坞、转接器或 KVM。笔记本请注明内置屏是否同时启用。
- HDR 是否开启、复现步骤和大致时间。请分别说明原生滑块是否出现、能否拖动，以及显示器实际亮度是否变化。
- 与 DisplayBrightness 有关的启动参数和 `DisplayBrightness-boot.txt`。在 `boot-args` 加上 `-dbbootlog`，重启后复现，再运行压缩包中的 `ExportBootLog.command`。报告会包含近期驱动日志和当前显示连接的 framebuffer、I²C 接口信息。
- 若无法启动，请用可用的 EFI 启动后采集报告，并附上失败那次的 OpenCore 日志（如有）。可用 `./ExportBootLog.command --opencore "/实际路径/opencore-log.txt"` 一并导出。若出现内核崩溃或冻结，请附上对应报告。Debug 包附带的 `.dSYM` 可用于解析崩溃堆栈。

分享前请检查报告，移除个人信息、序列号或敏感配置。

## 致谢

- 感谢 [Acidanthera 与 Lilu 贡献者](https://github.com/acidanthera/Lilu/graphs/contributors)提供 Lilu。
- 感谢 [OpenCorePkg](https://github.com/acidanthera/OpenCorePkg) 和 [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK) 提供启动集成和内核开发所需的头文件。
- 感谢 [Apple 开源的 IOGraphics](https://github.com/apple-oss-distributions/IOGraphics) 和 [MonitorControl](https://github.com/MonitorControl/MonitorControl) 提供显示架构与 DDC/CI 的实现参考。
- 感谢参与实机测试并分享连接配置和日志的社区用户。
