# 电子到位报信机构：固件、模型与工程图

[English](README.md) · [最新发布](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/latest) · [论文代码可用性表述](docs/CODE_AVAILABILITY.md)

## 资料入口

| 资料 | 位置 |
|---|---|
| 两份最新固件 | [wifi-once](firmware/wifi-once) · [wifi-retry](firmware/wifi-retry) · [固定v1.0.0发布版](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) |
| 机械模型与论文工程图 | [机械目录](%E6%9C%BA%E6%A2%B0/) · [工程图索引与PDF](%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/) |
| 原来上传的全部24个项目目录 | [原项目目录索引](docs/PROJECT_FILES.md) |

原项目目录和机械模型均保留在当前`main`分支中；`firmware/`中的两份最终工程作为最新固件版本。2026-10-10新增论文终稿对应的工程图，不改变已固定的`v1.0.0`固件发布版。

本仓库公开地面取心机构样机中电子报信与内管运动控制的STM32F1固件。SDM18提供测距数据，通过EN/STP/DIR接口控制步进驱动器；ESP8266 AT接口用于TCP状态传输、参数修改与控制指令。

**`v1.0.0`同时保留作者指定的两个最终固件版本。** 本次发布属于研究源码公开，不代表新增硬件试验。网络凭据改为本地配置模板，保留原控制逻辑和原工程设置。复现和进一步开发时需要了解的实现细节见[已知实现限制](docs/KNOWN_LIMITATIONS.md)。

## 两个版本如何选择

原文件夹的“一次”和“多次”指**WiFi连接尝试方式**，不是取心或钻进次数。两版均未实现上提至地面阈值后自动开始下一轮取心。

| 发布目录 | 原最终文件夹 | WiFi连接策略 | 到位默认阈值 | 地面/上提默认阈值 | 满心默认阈值 |
|---|---|---|---:|---:|---:|
| [wifi-once](firmware/wifi-once) | `SDM18测距机构-3wifi修改版（一次）` | 启动时尝试连接一次，此后不重试 | 360 mm | 700 mm | 70 mm |
| [wifi-retry](firmware/wifi-retry) | `SDM18测距机构-3wifi修改版（多次 ）` | 启动连接；连接标志为0时，按名义30 s间隔重试 | 400 mm | 800 mm | 100 mm |

共同设置包括：10项距离缓存、2 mm卡心极差阈值、名义100 ms控制间隔、到位后等待5 s、报警暂停3 s后反转上提。满心判断要求缓存值全部严格小于阈值；卡心判断采用缓存极差严格小于阈值。可修改参数只保存在RAM中。两版默认值与试验运行时通过指令设定的值应分别说明。

## 配置与构建

1. 下载固定发布版本或克隆仓库，选择其中一个工程。
2. 在该工程内，将`Core/Inc/wifi_config.example.h`复制为`Core/Inc/wifi_config.h`，填写自己的WiFi名称、密码、TCP服务器IP及端口。本地配置已加入Git忽略；示例占位值不能直接用于联网。
3. 用Keil µVision打开`MDK-ARM/SDM18-stm32-HAL.uvprojx`。保存的工程使用ARM Compiler **5.06 update 7（build 960）**、`STM32F103xE`宏和STM32F1器件包**2.4.1**。所需HAL/CMSIS源码已经保留；旧编译固件及个人IDE文件未纳入这两份规范整理的`firmware/`工程。
4. 根据实际板卡核对MCU、晶振、内存区域及Flash下载算法后编译；如需烧录，再单独配置相应的SWD下载器。
5. 在所配置的电脑/主机启动TCP服务器，再进行连接。这两份固件工程不包含原上位机程序或网页控制界面，通信方式见[接口与协议说明](docs/PROTOCOL.md)。

IOC和工程Device记录指向**STM32F103RC/xE**，但部分Flash/SVD设置仍指向C8，并保留历史ROM/RAM区域。本版保留这些原工程记录，不据此指定开发板型号。原源码个别注释沿用了较早阈值，实际默认值以执行宏和上表为准。本次具体做过的检查见[验证说明](docs/VALIDATION.md)。

## 内容与版本追溯

`firmware/`下为两份最新最终工程；`机械/`保留原16个模型，并在`机械/工程图/`增加PDF、PNG、SolidWorks原图及论文原图。原24个上传目录可通过[项目索引](docs/PROJECT_FILES.md)访问。`docs/`提供接线、协议、实现细节、验证范围和中英文论文引用示例。`SOURCE_MANIFEST.json`记录两份规范固件工程保留文件的原始及发布SHA-256值和变更原因，`CITATION.cff`提供固定固件版本的软件引用信息。

在两份规范整理的`firmware/`工程和固定`v1.0.0`发布版中，保留原文件的唯一修改，是把每版`Core/Src/main.c`的四项网络配置改为本地头文件引用；新增无凭据的`wifi_config.example.h`。未引用的库包、编译产物、IDE缓存及调试器配置未纳入这两份规范工程。恢复的原上传目录则保留原文件，包括原有的此类内容。没有改写控制算法，没有增加试验数据。

2026-10-10已从原提交`5d6e080df02d82a73eb847e18485b2bdb33146d4`将全部24个原上传目录原样恢复到`main`，原文件未改写，包括原最终版本文件夹快照和16个机械模型。原上传快照可能含旧网络配置，作者已确认旧WiFi配置不再使用。需要配置示例及发布说明时，使用固定`v1.0.0`发布版或当前`firmware/`中的两份最终工程。新增工程图位于`main`；既有`v1.0.0`标签和发布压缩包保持不变，其中不含本次新增工程图。

## 论文引用

建议在“代码可用性”或简短附录中引用固定的[v1.0.0发布页](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0)，避免仅链接会继续变化的main分支。GitHub可通过`CITATION.cff`显示软件引用。本仓库尚未分配软件DOI。

代码公开用于说明实现与支持复现；测距精度、识别性能、钻进效率、现场应用和空间适用性仍应依据论文及对应试验记录，不由仓库本身推定。可直接使用的中英文表述见[代码可用性文本](docs/CODE_AVAILABILITY.md)。

## 许可证

项目作者的原创贡献，包括原创代码、自绘工程图及发布文档，采用[MIT许可证](LICENSE)。厂商代码、随附模型和其他第三方资料保留原许可证及版权声明，根目录MIT不替代这些许可。恢复的原上传目录并非全部适用MIT。详见[第三方说明](THIRD_PARTY_NOTICES.md)。
