# Release validation / 发布验证

Checks below were performed for the `v1.0.0` publication snapshot on **2026-10-09**. They establish source packaging and build results, not new hardware performance.

## Source preservation and configuration

- Both retained original variants were compared with the author's local final directories. Before public packaging, the uploaded copies differed only in text line endings, with no substantive code differences.
- `SOURCE_MANIFEST.json` records original and release SHA-256 values for retained files. In each variant, only the four network macros in `Core/Src/main.c` are replaced with a local configuration include; a credential-free example header is added.
- All **33 source-file references** in each unchanged Keil project resolve within the published files.
- Real WiFi settings, historical compiled firmware, objects, personal IDE settings and debugger-specific files are excluded from the fixed `v1.0.0` release and the two curated `firmware/` projects. Each local `wifi_config.h` is ignored by Git. The original uploaded directories, restored to `main` on 2026-10-10, retain their historical files, including obsolete network settings that the owner confirmed are no longer in use.
- Original HAL/CMSIS licenses and file-level copyright notices are retained. The project MIT license is separate from those component licenses.

## Complete rebuild

The two publication variants were copied to separate build-check directories. In each copy, `wifi_config.example.h` was copied to `wifi_config.h` without inserting real credentials. Each project received one full Keil rebuild using the saved **ARM Compiler 5.06 update 7, build 960**. Both linked successfully and generated AXF/HEX in the temporary build directories. Source and project hashes remained unchanged. Those binaries are not release assets.

| Variant | Errors | Warnings | Result |
|---|---:|---:|---|
| `wifi-once` | 0 | 36 | Successful link; warnings retained |
| `wifi-retry` | 0 | 34 | Successful link; warnings retained |

Each variant reports 13 missing-final-newline warnings (`#1-D`, including repeated header inclusion). The remaining 23/21 warnings (`#870-D`) concern Chinese/Unicode or emoji `printf` string literals. They are not exclusively comment warnings. No claim is made that serial character rendering was verified. Warning-only µVision exit code was 1.

## Scope

The release checks did not flash hardware, move a motor, connect to the author's network, test TCP reconnection, measure timing, or repeat the paper's experiments. Successful compilation does not resolve the implementation details in [Known limitations](KNOWN_LIMITATIONS.md).

## 中文

本次核对确认：两最终工程与上传内容实质一致；每版33个Keil源码引用均存在；除四项网络配置拆分及新增示例头文件外，保留文件未改写控制逻辑。原凭据、旧编译固件及个人调试配置未纳入固定 `v1.0.0` 发布版和两份整理后的 `firmware/` 工程；2026-10-10 恢复到 `main` 的原上传目录保留原有历史文件。第三方许可与版权声明保留。

采用工程原定ARMCC 5.06 update 7（build 960），在独立副本中分别完整重编译一次。一次版0错误、36警告；重试版0错误、34警告，均成功链接。警告为末尾换行及中文/Unicode输出字符串，未自行改写。此次没有烧录和新增实物测试，不能以编译通过替代硬件、通信、时序或论文性能验证。

## Repository preservation checks — 2026-10-10 / 仓库资料完整性核对

The material restoration was published at commit [`c0c7b40`](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/commit/c0c7b40e8158e27fce74b14fde9e506d6e431814). The following file-preservation checks were recorded for that update:

- All 24 original project directories and their 16,277 files match the original snapshot in Git blobs, file modes and sizes, including the 16 original mechanical model files.
- All 542 files in the two final `firmware/` projects remain unchanged from the fixed release; the `v1.0.0` tag and release assets are unchanged.
- The 20 added drawing-related original files match their local source files in SHA-256 and size. Public downloads were checked against the same records; the published file list is in [DRAWING_MANIFEST.json](../%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/DRAWING_MANIFEST.json).
- The 108 local links in the seven documentation files prepared for the restoration resolved successfully.

These checks establish file preservation and access. They did not add hardware experiments, repeat manuscript measurements, or rebuild the SolidWorks assemblies and their external references.

此次资料恢复发布于上述提交。已核对：24 个原项目目录中的 16,277 个文件（含 16 个原机械模型）与原快照的 Git 对象、文件模式和大小一致；两份最终固件中的 542 个文件、固定标签和发布附件不变；20 份新增图纸相关原文件及公开下载内容的 SHA-256 和大小与原件一致；当次准备的七份文档共 108 个内部链接均有效。该核对说明文件完整性与可访问性，未新增实物试验、重做论文测量或重建 SolidWorks 装配体及其外部引用。

## Repeat the packaging check / 重复源码完整性检查

With Python 3, run `python tools/verify_release.py` from the extracted repository. It verifies the retained snapshot hashes, configuration extraction, licenses and all Keil source references without requiring a third-party Python package. This check does not compile the firmware or test hardware.

解压仓库后可用Python 3运行上述命令，核对保留文件哈希、配置拆分、许可及Keil源码引用；无需安装第三方Python包。该检查不编译固件，也不进行实物测试。
