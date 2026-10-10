# Manuscript correspondence and material availability / 论文对应关系与资料可用性

[Repository guide / 仓库说明](../README.md) · [中文说明](../README.zh-CN.md) · [Drawings / 工程图](../%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/)

The repository supplies code and mechanical design files accompanying the study. The manuscript describes the mechanism, experimental procedures, actual settings and reported results. The paragraphs below can be used in a code/design-availability section or a short appendix.

本仓库提供研究配套的代码与机械设计文件，机构原理、试验方法、实际设置和结果由论文阐述。以下简短表述可用于“代码与设计文件可用性”部分或附录。

## Relationship to the manuscript / 与论文的对应关系

The author has confirmed that the **main experiments reported in the manuscript used the single-attempt implementation**, provided as [wifi-once](../firmware/wifi-once/). [wifi-retry](../firmware/wifi-retry/) is retained as the other final implementation. Both source snapshots retain initialization parameters from different stages of the experimental work. For an individual reported test, use the settings stated in the manuscript and its corresponding record.

经作者确认，**论文主要试验采用“一次”版实现**，对应`wifi-once`；`wifi-retry`作为另一份最终实现保留。两份源码保留了不同时期试验的初始化设置。复现某项试验时，按论文及该试验记录设置参数。

The version names describe WiFi connection attempts. `wifi-once` attempts connection at startup; `wifi-retry` adds retries while the connection flag remains zero. The source-default landed/upper/core-full thresholds are 360/700/70 mm and 400/800/100 mm, respectively. These defaults identify the snapshots; the manuscript supplies the parameters of its reported experiments.

版本名称指WiFi建连尝试方式。一次版在启动时尝试连接，多次版在连接标志为0时继续尝试。两份源码到位/地面/满心初始化阈值分别为360/700/70 mm和400/800/100 mm。默认值用于说明所保留源码的设置，论文具体试验采用文中报告的参数。

Five parameters can be adjusted through runtime TCP commands: landed, upper/ground, core-full and suspected-jamming range thresholds, and the motor software-delay setting. Changes reside in RAM; restarting the MCU restores source defaults. The motor setting is a software-delay value, rather than a calibrated rpm value. Waiting times, buffer length and connection-retry timing are source constants. See [command names, ranges and state output](PROTOCOL.md).

到位、地面、满心、卡心极差阈值及电机软件延时参数可通过运行时TCP指令修改；修改值保存在RAM中，MCU重启后恢复源码默认值。电机参数表示软件延时，不能直接作为标定转速。等待时间、缓存长度及建连重试间隔为源码常量。指令名称、范围和状态报文见通信协议。

## Fixed material references / 固定资料版本

| Material / 资料 | Fixed reference / 固定引用 |
|---|---|
| Both final firmware snapshots / 两份最终固件 | [v1.0.0 source release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) |
| Original models / 原机械模型 | [Mechanical snapshot, 2026-10-10](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/tree/c0c7b40e8158e27fce74b14fde9e506d6e431814/%E6%9C%BA%E6%A2%B0) |
| Engineering drawings and original thesis figures / 工程图及论文原图 | [Drawing snapshot, 2026-10-10](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/tree/c0c7b40e8158e27fce74b14fde9e506d6e431814/%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE) |

The firmware release contains project files, required retained libraries, example network configuration and interface documentation. The mechanical snapshot contains the original 16 models and the added drawing set. The drawing set includes five PDFs, five PNG exports, five SolidWorks drawings, a same-source rack model and four original thesis images; [DRAWING_MANIFEST.json](../%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/DRAWING_MANIFEST.json) records their SHA-256 values. The pre-existing v1.0.0 firmware archives remain unchanged and do not contain the subsequently added mechanical drawing files.

固件发布版提供工程文件、保留的所需库、网络配置示例及接口说明。机械快照包含16个原模型和新增图纸集；图纸集包括5个PDF、5个PNG导出图、5个SolidWorks工程图、同源齿条模型及4幅论文原图，校验清单记录SHA-256。既有v1.0.0固件压缩包保持原样，后续补充的机械图纸通过上面的固定快照另行引用。

## 中文可用性表述

本研究配套的STM32控制与电子报信固件、机械模型及工程图已公开于[GitHub仓库](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism)。论文主要试验采用启动时单次WiFi连接的实现（`wifi-once`）；采用失败建连重试策略的另一份最终实现（`wifi-retry`）亦予保留。两份固件的固定发布版本为[v1.0.0](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0)，机械模型与工程图见[2026-10-10固定快照](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/tree/c0c7b40e8158e27fce74b14fde9e506d6e431814/%E6%9C%BA%E6%A2%B0)。源码初始化值保留了不同时期的试验设置，本文报告试验的实际参数以文中记录为准。作者原创代码、自绘资料及文档采用MIT许可证，第三方资料保留原许可与声明。

## English availability statement

The STM32 control and electronic-signaling firmware, mechanical models and engineering drawings supporting this study are available in the [GitHub repository](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism). The main experiments reported in the manuscript used the implementation that makes a single WiFi connection attempt at startup (`wifi-once`); the other final implementation, with failed-connection retries (`wifi-retry`), is also retained. Both firmware snapshots are identified by the fixed [v1.0.0 release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0), and the mechanical models and drawings by the [2026-10-10 snapshot](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/tree/c0c7b40e8158e27fce74b14fde9e506d6e431814/%E6%9C%BA%E6%A2%B0). Source initialization values retain settings from different stages of the experimental work; the actual parameters of the reported tests are those recorded in the manuscript. The author's original code, self-drawn material and documentation are licensed under MIT; third-party material retains its original terms and notices.

## Suggested software reference / 固件参考文献

Wang, Z. (2026). Electronic position signaling and inner-tube control firmware (Version 1.0.0) [Computer software]. GitHub. https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0

`CITATION.cff` describes this fixed software release. The separate mechanical snapshot can be identified by its commit and link in the availability statement. No software DOI or published-paper bibliographic details have been assigned in this repository.

`CITATION.cff`描述固定固件版本；机械资料在可用性声明中以提交快照及链接标识。仓库未填写软件DOI或尚未确定的已发表论文信息。
