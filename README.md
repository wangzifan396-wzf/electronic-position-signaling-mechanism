# Supporting research materials: electronic position-signaling mechanism

[中文说明](README.zh-CN.md) · [Final firmware release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) · [Manuscript correspondence and availability](docs/CODE_AVAILABILITY.md)

This repository provides firmware, mechanical models and engineering drawings accompanying a ground-based prototype study of an electronic position-signaling mechanism for coring. The implementation combines SDM18 distance sensing, stepper-driven inner-tube motion and ESP8266 TCP communication for position signaling, core-full and suspected-jamming alarms, and retrieval control.

**The main experiments reported in the manuscript used the single-attempt implementation (`wifi-once`).** Both author-designated final implementations are retained. Experimental procedures, actual parameter settings and reported results are documented in the manuscript; use its settings when reproducing a reported test.

## Materials

| Material | Entry point |
|---|---|
| Main experimental implementation | [firmware/wifi-once](firmware/wifi-once/) |
| Final implementation with failed-connection retries | [firmware/wifi-retry](firmware/wifi-retry/) |
| Mechanical models and original drawings | [Mechanical directory](%E6%9C%BA%E6%A2%B0/) · [Five drawing groups and thesis figures](%E6%9C%BA%E6%A2%B0/%E5%B7%A5%E7%A8%8B%E5%9B%BE/) |
| Original development projects | [Index of all 24 original directories](docs/PROJECT_FILES.md) |
| Citation and manuscript wording | [Code and design availability](docs/CODE_AVAILABILITY.md) |

The original directories and 16 mechanical models are retained in `main`. The drawing directory contains the original PDF/PNG exports, SolidWorks drawings and four engineering figures extracted unchanged from the thesis. The two final firmware implementations remain under `firmware/`.

## Firmware variants and experimental parameters

“Once” and “retry” refer to **WiFi connection attempts**. `wifi-once` attempts connection during startup; `wifi-retry` additionally retries at a nominal 30 s interval while the connection flag remains zero. These names do not indicate the number of coring cycles. [Implementation notes](docs/KNOWN_LIMITATIONS.md) explain the connection behavior in detail.

The two retained source snapshots contain initialization settings from different stages of the experimental work. The following values are **source defaults**; parameters for a reported experiment follow the corresponding manuscript record.

| Final source snapshot | Original final folder | Landed default | Upper/ground default | Core-full default |
|---|---|---:|---:|---:|
| [wifi-once](firmware/wifi-once/) | `SDM18测距机构-3wifi修改版（一次）` | 360 mm | 700 mm | 70 mm |
| [wifi-retry](firmware/wifi-retry/) | `SDM18测距机构-3wifi修改版（多次 ）` | 400 mm | 800 mm | 100 mm |

Landed, upper/ground, core-full and suspected-jamming thresholds, together with the motor software-delay setting, can be adjusted through TCP commands. `GET_PARAMS` returns the current settings. Changes are held in RAM; an MCU restart restores the source defaults. Timing and buffer settings are defined in the source and require recompilation to change. See the [command ranges and interface definitions](docs/PROTOCOL.md).

## Using the materials

1. For the main manuscript experiments, start with `firmware/wifi-once/` and configure the parameters reported for the relevant test.
2. Create the local network configuration from `Core/Inc/wifi_config.example.h`. Follow the [build and configuration guide](docs/BUILD.md) for the saved Keil project and board settings.
3. Use the drawing index to view full-sheet PDFs and locate the corresponding native CAD files. The PDFs can be read independently; opening native files may require locating their original model references.

[Source and build checks](docs/VALIDATION.md) document the checks performed for the published materials. [Implementation notes](docs/KNOWN_LIMITATIONS.md) retain the details needed to interpret and extend the original firmware.

## Cite and identify the materials

The fixed [v1.0.0 release](https://github.com/wangzifan396-wzf/electronic-position-signaling-mechanism/releases/tag/v1.0.0) identifies the two final firmware snapshots. Models and drawings are also available in the repository; the [availability guide](docs/CODE_AVAILABILITY.md) provides a fixed drawing snapshot and short Chinese/English manuscript statements. The existing v1.0.0 archives contain the firmware, while the engineering drawings added on 2026-10-10 are available separately in `main`.

`CITATION.cff` supplies the versioned software citation. The author's original code, self-drawn material and documentation use the [MIT license](LICENSE); third-party components retain their own licenses and notices. See [Third-party notices](THIRD_PARTY_NOTICES.md).
