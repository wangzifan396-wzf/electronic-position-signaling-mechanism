# Release validation / 发布验证

Checks below were performed for the `v1.0.0` publication snapshot on **2026-10-09**. They establish source packaging and build results, not new hardware performance.

## Source preservation and configuration

- Both retained original variants were compared with the author's local final directories. Before public packaging, the uploaded copies differed only in text line endings, with no substantive code differences.
- `SOURCE_MANIFEST.json` records original and release SHA-256 values for retained files. In each variant, only the four network macros in `Core/Src/main.c` are replaced with a local configuration include; a credential-free example header is added.
- All **33 source-file references** in each unchanged Keil project resolve within the published files.
- Real WiFi settings, historical compiled firmware, objects, personal IDE settings and debugger-specific files are excluded from this release snapshot. Each local `wifi_config.h` is ignored by Git. Earlier repository commits are retained at the owner's request and may contain obsolete network settings that the owner confirmed are no longer in use.
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

本次核对确认：两最终工程与上传内容实质一致；每版33个Keil源码引用均存在；除四项网络配置拆分及新增示例头文件外，保留文件未改写控制逻辑。原凭据、旧编译固件及个人调试配置未纳入公开发布，第三方许可保留。

采用工程原定ARMCC 5.06 update 7（build 960），在独立副本中分别完整重编译一次。一次版0错误、36警告；重试版0错误、34警告，均成功链接。警告为末尾换行及中文/Unicode输出字符串，未自行改写。此次没有烧录和新增实物测试，不能以编译通过替代硬件、通信、时序或论文性能验证。

## Repeat the packaging check / 重复源码完整性检查

With Python 3, run `python tools/verify_release.py` from the extracted repository. It verifies the retained snapshot hashes, configuration extraction, licenses and all Keil source references without requiring a third-party Python package. This check does not compile the firmware or test hardware.

解压仓库后可用Python 3运行上述命令，核对保留文件哈希、配置拆分、许可及Keil源码引用；无需安装第三方Python包。该检查不编译固件，也不进行实物测试。
