# Code availability / 代码可用性

Use the fixed release link for the submitted manuscript. A short code-availability statement is usually sufficient; an appendix is useful if variant selection or parameter mapping also needs explanation. Adapt the heading and placement to the chosen journal. These paragraphs are supplied for insertion into the manuscript; the paper itself has not been edited by this repository release.

## 中文示例

本研究相关的STM32控制与电子报信固件已公开于GitHub（https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism），固定发布版本为v1.0.0（https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0）。该版本包含启动时单次WiFi连接和失败后定时重试两种最终实现，并提供工程文件、默认参数、接口协议及本地网络配置示例。项目作者的原创代码采用MIT许可证，第三方组件保留原许可证。

## English example

The STM32 control and electronic signaling firmware associated with this study is available on GitHub (https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism), with a fixed source release at version v1.0.0 (https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0). The release contains the two final implementations, using either a single startup WiFi connection attempt or periodic retries after failed connection attempts, together with project files, default parameters, interface documentation and an example local network configuration. The project author's original code is licensed under MIT; third-party components retain their original licenses.

## Optional appendix note / 可选附录说明

The `wifi-once` defaults are landed/upper/core-full thresholds of 360/700/70 mm; `wifi-retry` uses 400/800/100 mm. Both accept RAM-only parameter changes. Describe the variant and runtime settings actually used in each reported experiment; do not assume that publishing both variants proves that both were used in every experiment. The manuscript's 65 mm distance with a 70 mm core-full threshold is consistent with the stated 70 mm runtime threshold; the other variant's 100 mm default should not replace it.

`wifi-once`的到位/地面/满心默认阈值为360/700/70 mm，`wifi-retry`为400/800/100 mm。两版均支持运行时修改RAM参数。论文应依据试验记录说明实际使用的版本和运行参数；公开两版不表示每项试验都使用了两版。文中65 mm距离、70 mm满心阈值的记录应保留其实际设定，不用另一版100 mm默认值替换。

## Suggested software reference

Wang, Z. (2026). Electronic position signaling and inner-tube control firmware (Version 1.0.0) [Computer software]. GitHub. https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0

No DOI or published-paper bibliographic details have been invented. GitHub releases are versioned references; an archival DOI, if later obtained, can be added separately. [GitHub release documentation](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases) explains that releases are based on Git tags.
