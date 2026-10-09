# Electronic position signaling and inner-tube control firmware

[中文说明](README.zh-CN.md) · [Latest release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/latest) · [Code availability text](docs/CODE_AVAILABILITY.md)

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
3. Open `MDK-ARM/SDM18-stm32-HAL.uvprojx` in Keil µVision. The saved project uses ARM Compiler **5.06 update 7, build 960**, the `STM32F103xE` definition, and the STM32F1 device pack **2.4.1**. Required HAL/CMSIS sources are included; old binaries and personal IDE files are excluded.
4. Check the target MCU, clock, memory map and Flash algorithm against the actual board, then build. Configure a matching SWD programmer separately if downloading to hardware.
5. Run a TCP server on the configured host before connection attempts. These two firmware folders do not contain the original host application or a browser interface. See [Protocol and interface guide](docs/PROTOCOL.md).

The IOC and project Device records indicate **STM32F103RC/xE**, but some saved Flash/SVD entries still refer to C8, and the project uses historical ROM/RAM regions. These records are preserved in this snapshot and do not identify a particular development board. Source comments can contain older thresholds: use the executable macro definitions and the table above. See [Validation](docs/VALIDATION.md) for checks actually performed for this release.

## Repository contents

```text
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

The source-to-release manifest records the retained files. The only change to each retained original file is the extraction of four network macros in `Core/Src/main.c` into a local configuration include; `wifi_config.example.h` is new. Unused library packages, generated binaries, IDE caches and debugger-specific settings are omitted. No firmware algorithms were changed, and no new experiments were added.

Earlier uploads remain in the repository's Git history as development records. They are not the current release and may contain obsolete network configuration; the owner has confirmed that the old WiFi configuration is no longer in use. Use the fixed release or current `firmware/` folders for these two final source versions.

## Cite this software

Use the fixed [v1.0.0 release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) when describing this snapshot in a manuscript. `CITATION.cff` also enables GitHub’s citation interface. No software DOI has been assigned.

> Wang, Z. (2026). Electronic position signaling and inner-tube control firmware (Version 1.0.0) [Computer software]. GitHub. https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0

The repository documents firmware implementation. It does not itself establish sensor accuracy, drilling efficiency, classification accuracy, field performance or space qualification. Corresponding measurements should be cited from the experiment records or manuscript.

## License

The project author’s original contributions and release documentation are licensed under [MIT](LICENSE). STMicroelectronics and Arm components retain their original licenses and copyright notices; the root MIT license does not replace them. See [Third-party notices](THIRD_PARTY_NOTICES.md).
