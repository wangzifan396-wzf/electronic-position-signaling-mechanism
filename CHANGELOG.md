# Changelog

## Paper-supporting documentation update — 2026-10-10

- Reframed the repository overview and navigation around the manuscript's supporting firmware, mechanical models and engineering drawings.
- Recorded the author's confirmation that the manuscript's main experiments primarily used `firmware/wifi-once`; `firmware/wifi-retry` remains an alternative final source snapshot from a different experimental period.
- Clarified that the snapshots retain their respective startup defaults, while the manuscript reports the experimental settings to use for reproduction. Documented the five RAM-only settings supported by runtime commands and distinguished them from compile-time constants.
- Updated the code/material availability wording and clarified which materials are in the fixed firmware release and which were added to `main`.
- This update changes documentation only. Original source code, startup defaults, historical project files, models, drawings, the `v1.0.0` tag and release assets are unchanged.

中文：按论文配套研究资料优化仓库说明与入口；记录作者确认的主要试验版本为一次连接版，重试连接版保留另一实验阶段的最终源码。两版初始化默认值原样保留，复现参数以论文为准；明确五项运行时 RAM 参数与编译期常量的区别。此次仅修改说明文档，源码、默认值、历史项目、模型、图纸及固定固件标签和发布附件不变。

## Repository material update — 2026-10-10

- Restored all 24 originally uploaded root directories from commit `5d6e080df02d82a73eb847e18485b2bdb33146d4`, preserving their files unchanged, including all 16 original mechanical model files.
- Kept the two curated final firmware projects in `firmware/wifi-once` and `firmware/wifi-retry` as the latest firmware. The restored original final-version folders are the original upload snapshots.
- Added the author's thesis drawing set under `机械/工程图`: five original PDFs, five original PNGs, five original SolidWorks drawings, the same-source rack model and four original thesis images, with SHA-256 values in `DRAWING_MANIFEST.json`.
- Added bilingual mechanical/drawing guides and an index of all original project directories. Clarified the MIT and third-party license scope across curated firmware, restored uploads and drawings.
- This is a `main`-branch repository update. The `v1.0.0` tag, release assets, software citation metadata and two curated firmware projects are unchanged; the new drawing material is not in the existing `v1.0.0` archives.

中文：全部24个原目录及16个机械模型已原样恢复；两份最终固件继续作为最新版。新增论文终稿对应的工程图及索引说明，原文件未重绘。此次仅更新仓库`main`资料，既有`v1.0.0`标签、发布附件、软件引用信息及规范固件工程不变。

## v1.0.0 — 2026-10-09

- First curated public source release containing both author-designated final variants: `wifi-once` and `wifi-retry`.
- Retained the final control logic, thresholds, interfaces, source files and Keil/CubeMX project records.
- Extracted hardcoded network settings into an ignored local header and added an example configuration; excluded historical binaries and personal IDE/debug files.
- Added bilingual documentation, protocol and implementation notes, software citation metadata, source hashes and MIT/third-party license notices.
- This is a publication snapshot, not a claim of new experiments or fixes to the original control implementation.
