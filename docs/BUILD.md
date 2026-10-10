# Build and configuration / 构建与配置

[Repository guide / 仓库说明](../README.md) · [Manuscript correspondence / 论文对应关系](CODE_AVAILABILITY.md) · [Commands / 通信指令](PROTOCOL.md)

Use one of the final projects in `firmware/`. The main experiments reported in the manuscript used `wifi-once`; `wifi-retry` is also retained as an author-designated final implementation. Configure the actual parameters of the test being reproduced from the manuscript. The source-default differences reflect settings retained from different experimental periods.

使用`firmware/`中的一份最终工程。论文主要试验采用`wifi-once`，`wifi-retry`作为另一份最终实现保留。复现时按论文中对应试验的记录配置参数；两份源码默认值来自不同阶段的试验设置。

## Configure and build

1. Download the fixed [v1.0.0 firmware release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) or clone the repository, then select one variant under `firmware/`.
2. In that variant, copy `Core/Inc/wifi_config.example.h` to `Core/Inc/wifi_config.h`. Set the WiFi SSID/password and TCP server IP/port. The local header is ignored by Git; the example contains placeholders.
3. Open `MDK-ARM/SDM18-stm32-HAL.uvprojx` in Keil µVision. The saved project uses ARM Compiler **5.06 update 7, build 960**, the `STM32F103xE` definition and STM32F1 device pack **2.4.1**. Required HAL/CMSIS sources are included. Historical binaries and personal IDE settings are excluded from the two curated `firmware/` projects and the fixed release; the original development directories retain their uploaded contents.
4. Check the MCU, clock, memory map and Flash algorithm against the actual board before building. Configure a matching SWD programmer separately if downloading to hardware.
5. Start a TCP server on the configured host before the connection attempt. These two firmware folders do not contain the original host application or a browser interface. See [Interfaces and text protocol](PROTOCOL.md).

## 中文步骤

1. 下载固定v1.0.0固件发布版或克隆仓库，在`firmware/`中选择一份工程。
2. 将该工程的`Core/Inc/wifi_config.example.h`复制为`Core/Inc/wifi_config.h`，填写自己的WiFi名称、密码、TCP服务器IP和端口。本地头文件已加入Git忽略，示例占位值需自行替换。
3. 用Keil µVision打开`MDK-ARM/SDM18-stm32-HAL.uvprojx`。原工程使用ARM Compiler **5.06 update 7（build 960）**、`STM32F103xE`宏及STM32F1器件包**2.4.1**。HAL/CMSIS源码已保留。两份整理工程及固定发布版不含历史编译产物和个人IDE配置；原开发目录保留原上传内容。
4. 按实物核对MCU、晶振、内存区域和Flash下载算法后编译；需烧录时另行配置对应的SWD下载器。
5. 在所配置的主机启动TCP服务器，再进行连接。两份固件目录不包含原上位机程序或网页控制界面，指令及报文格式见通信协议。

## Original project settings / 原工程设置

The IOC and project Device records indicate **STM32F103RC/xE**, while some Flash/SVD settings still refer to C8 and historical ROM/RAM regions are retained. These are the original saved settings; check them against the actual MCU. Some source comments contain earlier thresholds, so read the executable definitions and current RAM settings when inspecting a run.

IOC和工程Device记录指向**STM32F103RC/xE**，部分Flash/SVD设置仍指向C8，并保留历史ROM/RAM区域。上述原工程设置需按实际MCU核对。源码个别注释保留了早期阈值，检查运行状态时应读取执行定义及当前RAM参数。

[Validation records](VALIDATION.md) describe the full rebuilds actually performed. [Implementation notes](KNOWN_LIMITATIONS.md) document the original control behavior. `SOURCE_MANIFEST.json` records the retained firmware files and the extraction of four network macros into the local configuration header; control algorithms were preserved.

验证记录提供已执行的完整重编译结果；实现说明记录原控制行为。`SOURCE_MANIFEST.json`记录保留文件及四项网络配置拆分，控制算法保持原样。
