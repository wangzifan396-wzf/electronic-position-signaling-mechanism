# v1.0.0 — Final firmware source release

This release contains the two author-designated final STM32F1 firmware versions for electronic position signaling and inner-tube control:

- `wifi-once`: one startup WiFi connection attempt; default landed/upper/core-full thresholds of 360/700/70 mm.
- `wifi-retry`: periodic retries while the connection flag is zero; defaults of 400/800/100 mm.

The control logic is retained. Network credentials are supplied through a local ignored configuration file. Each source archive includes the required Keil/CubeMX files, retained HAL/CMSIS sources, license notices, configuration example and source manifest. Historical binaries and personal debugger/IDE files are excluded.

See the README and `docs/VALIDATION.md` for actual build checks and `docs/KNOWN_LIMITATIONS.md` for implementation details. The release does not introduce new hardware test results. The original code's limitations have not been silently repaired.

## 中文

首次整理公开作者指定的两个最终固件版本，分别采用启动时单次WiFi连接与连接失败后的定时重试策略。默认参数分别列明；控制逻辑保持原最终版。网络配置改为本地文件和示例模板；提供中英文README、接口说明、源码追溯和软件引用信息。原创贡献采用MIT，第三方组件保留原许可。

论文引用建议指向本发布页。两份独立源码压缩包对应两个版本，不含原WiFi密码和历史编译固件。
