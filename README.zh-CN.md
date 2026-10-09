# 电子到位报信与内管运动控制固件

[English](README.md) · [最新发布](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/latest) · [论文代码可用性表述](docs/CODE_AVAILABILITY.md)

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
3. 用Keil µVision打开`MDK-ARM/SDM18-stm32-HAL.uvprojx`。保存的工程使用ARM Compiler **5.06 update 7（build 960）**、`STM32F103xE`宏和STM32F1器件包**2.4.1**。所需HAL/CMSIS源码已经保留，旧编译固件及个人IDE文件未纳入发布。
4. 根据实际板卡核对MCU、晶振、内存区域及Flash下载算法后编译；如需烧录，再单独配置相应的SWD下载器。
5. 在所配置的电脑/主机启动TCP服务器，再进行连接。这两份固件工程不包含原上位机程序或网页控制界面，通信方式见[接口与协议说明](docs/PROTOCOL.md)。

IOC和工程Device记录指向**STM32F103RC/xE**，但部分Flash/SVD设置仍指向C8，并保留历史ROM/RAM区域。本版保留这些原工程记录，不据此指定开发板型号。原源码个别注释沿用了较早阈值，实际默认值以执行宏和上表为准。本次具体做过的检查见[验证说明](docs/VALIDATION.md)。

## 内容与版本追溯

`firmware/`下为两份最终工程；`docs/`提供接线、协议、实现细节、验证范围和中英文论文引用示例。`SOURCE_MANIFEST.json`记录保留文件的原始及发布SHA-256值和变更原因，`CITATION.cff`提供软件引用信息。

保留原文件中的唯一修改，是把每版`Core/Src/main.c`的四项网络配置改为本地头文件引用；新增无凭据的`wifi_config.example.h`。未引用的库包、编译产物、IDE缓存及调试器配置未纳入发布。没有改写控制算法，没有增加试验数据。

较早上传的工程保留在Git提交历史中作为开发记录，不作为当前发布版本；历史文件可能包含旧网络配置，作者已确认旧WiFi配置不再使用。获取这两个最终版本时，以固定发布版本或当前`firmware/`目录为准。

## 论文引用

建议在“代码可用性”或简短附录中引用固定的[v1.0.0发布页](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0)，避免仅链接会继续变化的main分支。GitHub可通过`CITATION.cff`显示软件引用。本仓库尚未分配软件DOI。

代码公开用于说明实现与支持复现；测距精度、识别性能、钻进效率、现场应用和空间适用性仍应依据论文及对应试验记录，不由仓库本身推定。可直接使用的中英文表述见[代码可用性文本](docs/CODE_AVAILABILITY.md)。

## 许可证

项目作者的原创贡献和发布文档采用[MIT许可证](LICENSE)。STMicroelectronics、Arm等第三方组件保留原许可证与版权声明，根目录MIT不替代这些许可。详见[第三方说明](THIRD_PARTY_NOTICES.md)。
