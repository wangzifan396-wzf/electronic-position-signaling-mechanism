# 论文配套研究资料：电子到位报信机构

[English](README.md) · [最终固件发布版](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) · [论文对应关系与可用性说明](docs/CODE_AVAILABILITY.md)

本仓库提供电子到位报信机构地面样机研究的配套固件、机械模型与工程图。系统结合SDM18测距、步进电机驱动的内管运动及ESP8266 TCP通信，实现到位报信、满心与卡心报警和上提控制。

**论文主要试验采用“一次”版实现，即`wifi-once`。** 作者保留的两份最终实现均已公开。机构原理、试验方法、实际参数与结果以论文及对应试验记录为准，复现具体试验时按论文设置参数。

## 资料入口

| 资料 | 位置 |
|---|---|
| 论文主要试验采用的实现 | [firmware/wifi-once](firmware/wifi-once/) |
| 带失败建连重试的另一份最终实现 | [firmware/wifi-retry](firmware/wifi-retry/) |
| 机械模型及原始工程图 | [机械目录](%E6%9C%BA%E6%A2%B0/) · [五组工程图与论文原图](%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/) |
| 原始开发工程 | [全部24个原目录索引](docs/PROJECT_FILES.md) |
| 论文引用及附录表述 | [代码与设计文件可用性说明](docs/CODE_AVAILABILITY.md) |

原项目目录和16个机械模型均保留在`main`中。工程图目录提供原PDF、PNG、SolidWorks工程图，以及从终稿原样提取的四幅工程图图片。两份最终固件集中在`firmware/`下。

## 固件版本与试验参数

“一次”和“多次”指**WiFi建连尝试方式**。`wifi-once`在启动时尝试连接一次；`wifi-retry`在启动连接之外，连接标志为0时按名义30 s间隔重试。名称不表示取心次数。具体连接行为见[实现说明](docs/KNOWN_LIMITATIONS.md)。

两份保留源码中的初始化参数来自**不同阶段的试验设置**。下表列出源码默认值；论文中具体试验的参数，以对应试验记录为准。

| 最终源码目录 | 原最终文件夹 | 到位默认阈值 | 地面/上提默认阈值 | 满心默认阈值 |
|---|---|---:|---:|---:|
| [wifi-once](firmware/wifi-once/) | `SDM18测距机构-3wifi修改版（一次）` | 360 mm | 700 mm | 70 mm |
| [wifi-retry](firmware/wifi-retry/) | `SDM18测距机构-3wifi修改版（多次 ）` | 400 mm | 800 mm | 100 mm |

到位、地面、满心、卡心极差阈值和电机软件延时参数支持通过TCP指令修改，可用`GET_PARAMS`查询当前设置。修改值保存在RAM中，MCU重启后恢复源码默认值。等待时间、缓存长度等在源码中定义，修改后需重新编译。指令范围及接口定义见[通信协议](docs/PROTOCOL.md)。

## 使用资料

1. 复现论文主要试验时，从`firmware/wifi-once/`开始，按对应试验的记录配置参数。
2. 根据`Core/Inc/wifi_config.example.h`建立自己的本地网络配置，再按[构建与配置说明](docs/BUILD.md)使用原Keil工程并核对板卡设置。
3. 通过工程图索引查看完整PDF并定位CAD原文件。PDF可独立阅读，打开CAD时可能需要重新定位其原模型引用。

[源码与构建核对记录](docs/VALIDATION.md)说明本次公开资料已完成的检查；[实现说明](docs/KNOWN_LIMITATIONS.md)保留理解和继续开发原固件所需的技术细节。

## 论文引用与资料版本

固定的[v1.0.0发布版](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0)标识两份最终固件快照。模型和工程图在仓库中另行提供，[可用性说明](docs/CODE_AVAILABILITY.md)给出了工程图固定快照及可用于论文正文或附录的中英文简短表述。既有v1.0.0压缩包包含固件，2026-10-10补充的工程图保存在`main`中。

`CITATION.cff`提供固定固件版本的软件引用信息。作者原创代码、自绘资料和文档采用[MIT许可证](LICENSE)；第三方组件保留各自原许可与声明，详见[第三方说明](THIRD_PARTY_NOTICES.md)。
