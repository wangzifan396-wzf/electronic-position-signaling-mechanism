# Changelog

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
