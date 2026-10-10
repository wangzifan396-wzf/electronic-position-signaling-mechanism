# Electronic position signaling mechanism: firmware, models and drawings

[中文说明](README.zh-CN.md) · [Latest release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/latest) · [Code availability text](docs/CODE_AVAILABILITY.md)

## Start here

| Material | Location |
|---|---|
| Two latest firmware variants | [wifi-once](firmware/wifi-once) · [wifi-retry](firmware/wifi-retry) · [fixed v1.0.0 release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) |
| Mechanical models and thesis engineering drawings | [Mechanical directory](%E6%9C%BA%E6%A2%B0/) · [Drawing index and PDFs](%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/) |
| All 24 original uploaded project directories | [Original project index](docs/PROJECT_FILES.md) |

The original project directories, including the mechanical models, are present in the current `main` branch. The two final variants in `firmware/` remain the latest firmware. The engineering drawings added on 2026-10-10 correspond to the author's thesis drawing set; this addition does not change the fixed `v1.0.0` firmware release.

STM32F1 research firmware for the electronic signaling and inner-tube transport functions of a ground-based coring-mechanism prototype. An SDM18 ranging module supplies distance data; EN/STP/DIR outputs command a stepper driver; an ESP8266 AT interface provides TCP status transmission and parameter/control commands.

**Version `v1.0.0` contains both author-designated final firmware variants.** It is a source snapshot for research transparency, rather than a new hardware-validation result. Local WiFi credentials have been replaced by a configuration template. The control logic and original project settings are preserved. Implementation details that matter when reproducing or extending the firmware are listed in [Known limitations](docs/KNOWN_LIMITATIONS.md).

## Select a variant

“Once” and “retry” describe **WiFi connection attempts**, not the number of drilling or coring cycles. Neither variant automatically restarts a new coring cycle after reaching the upper threshold.

| Variant | Original final folder | Connection behavior | Landed threshold | Upper/ground threshold | Core-full threshold |
|---|---|---|---:|---:|---:|
| [wifi-once](firmware/wifi-once) | `SDM18测距机构-3wifi修改版（一次）` | One connection attempt during startup; no later retry | 360 mm | 700 mm | 70 mm |
| [wifi-retry](firmware/wifi-retry) | `SDM18测距机构-3wifi修改版（多次 ）` | Startup attempt; nominal 30 s retry interval while the connection flag is zero | 400 mm | 800 mm | 100 mm |

Both variants use a 10-entry distance buffer, a 2 mm suspected-jamming range threshold, a nominal 100 ms control interval, a 5 s landed wait, and a 3 s alarm pause before retrieval. Core-full detection requires all buffered values to be strictly below its threshold; suspected jamming uses a buffered range strictly below its threshold. Parameters are held in RAM and can be changed through text commands. Defaults in one variant should not be substituted for the experimental settings of the other.

## Build and configure

1. Download the tagged source archive or clone this repository. Choose **one** variant.
2. Copy `Core/Inc/wifi_config.example.h` to `Core/Inc/wifi_config.h` within that variant. Set the WiFi SSID/password and TCP server IP/port. The local file is ignored by Git; the placeholder example is not a working network configuration.
3. Open `MDK-ARM/SDM18-stm32-HAL.uvprojx` in Keil µVision. The saved project uses ARM Compiler **5.06 update 7, build 960**, the `STM32F103xE` definition, and the STM32F1 device pack **2.4.1**. Required HAL/CMSIS sources are included; old binaries and personal IDE files are excluded from these two curated `firmware/` projects.
4. Check the target MCU, clock, memory map and Flash algorithm against the actual board, then build. Configure a matching SWD programmer separately if downloading to hardware.
5. Run a TCP server on the configured host before connection attempts. These two firmware folders do not contain the original host application or a browser interface. See [Protocol and interface guide](docs/PROTOCOL.md).

The IOC and project Device records indicate **STM32F103RC/xE**, but some saved Flash/SVD entries still refer to C8, and the project uses historical ROM/RAM regions. These records are preserved in this snapshot and do not identify a particular development board. Source comments can contain older thresholds: use the executable macro definitions and the table above. See [Validation](docs/VALIDATION.md) for checks actually performed for this release.

## Repository contents

```text
机械/                    Restored original models and engineering-drawing index
机械/工程图/              Original PDF, PNG, SolidWorks drawings and thesis figures
docs/PROJECT_FILES.md    Index of all 24 restored original directories
firmware/wifi-once/       Final variant with one startup WiFi attempt
firmware/wifi-retry/      Final variant with failed-connection retries
docs/PROTOCOL.md         Pin assignments, commands and status format
docs/KNOWN_LIMITATIONS.md Implementation details and reproduction limits
docs/CODE_AVAILABILITY.md Chinese/English manuscript wording
docs/VALIDATION.md       Release checks and their scope
SOURCE_MANIFEST.json     Original/release SHA-256 values and packaging changes
CITATION.cff             Versioned software citation
THIRD_PARTY_NOTICES.md   Licenses and retained notices
```

The source-to-release manifest records the retained files in the two curated `firmware/` projects and the fixed `v1.0.0` release. In those projects, the only change to each retained original file is the extraction of four network macros in `Core/Src/main.c` into a local configuration include; `wifi_config.example.h` is new. Unused library packages, generated binaries, IDE caches and debugger-specific settings are omitted from those curated projects. The restored original directories keep their original files, including any such artifacts. No firmware algorithms were changed, and no new experiments were added.

On 2026-10-10, all 24 original uploaded directories were restored to `main` from commit `5d6e080df02d82a73eb847e18485b2bdb33146d4`, without modifying their original files. This includes the original final-version folder snapshots and 16 original mechanical model files. Their contents can be browsed through the [project index](docs/PROJECT_FILES.md). Original snapshots may contain obsolete network configuration; the owner has confirmed that the old WiFi configuration is no longer in use. For the two final firmware versions with example configuration and release documentation, use the current `firmware/` folders or the fixed `v1.0.0` release. The added engineering drawings are on `main`, not in the unchanged `v1.0.0` tag or release archives.

## Cite this software

Use the fixed [v1.0.0 release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) when describing this snapshot in a manuscript. `CITATION.cff` also enables GitHub’s citation interface. No software DOI has been assigned.

> Wang, Z. (2026). Electronic position signaling and inner-tube control firmware (Version 1.0.0) [Computer software]. GitHub. https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0

The repository documents firmware implementation. It does not itself establish sensor accuracy, drilling efficiency, classification accuracy, field performance or space qualification. Corresponding measurements should be cited from the experiment records or manuscript.

## License

The project author’s original contributions, including original code, self-drawn engineering figures and release documentation, are licensed under [MIT](LICENSE). Vendor code, supplied models and other third-party materials retain their original licenses and notices; the root MIT license does not replace them. The restored original directories are not uniformly MIT-licensed. See [Third-party notices](THIRD_PARTY_NOTICES.md).
