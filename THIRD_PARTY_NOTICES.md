# Third-party licenses and retained notices

The root `LICENSE` applies to the project author's original contributions and release documentation. It does not relicense generated/vendor code or remove any original copyright notice. Files with their own notice remain governed by that notice; original notices have been retained unchanged.

The following table describes the two curated projects under `firmware/` and the fixed `v1.0.0` release.

| Retained component | Applicable original license/notice | Included location in each variant |
|---|---|---|
| Arm CMSIS headers | Apache License 2.0 | `Drivers/CMSIS/LICENSE.txt`, `Drivers/CMSIS/Include/` |
| ST/Arm STM32F1 CMSIS Device | Apache License 2.0 as specified by its supplied license | `Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt` |
| ST STM32F1 HAL/LL driver | BSD-3-Clause when received without a package license, as specified by its supplied license | `Drivers/STM32F1xx_HAL_Driver/LICENSE.txt`; full BSD text also in `LICENSES/ST-BSD-3-Clause.txt` |
| ST generated configuration and startup scaffolding | Original file-level ST notices, including BSD terms where stated | `Core/`, `MDK-ARM/startup_stm32f103xe.s` |

The official [STM32CubeF1 component license table](https://github.com/STMicroelectronics/STM32CubeF1/blob/master/LICENSE.md) lists CMSIS and CMSIS Device under Apache-2.0 and STM32F1 HAL under BSD-3-Clause. The added full BSD text was retrieved from the [official HAL driver license](https://github.com/STMicroelectronics/stm32f1xx-hal-driver/blob/master/LICENSE.md); the original bundled license files are retained as well.

No additional author or license has been invented for helper files that contain no separate notice. The two curated `firmware/` projects and the fixed `v1.0.0` release omit toolchain executables, proprietary device packs, original binaries and personal debugger configuration. This omission statement does not describe the restored original project directories, which retain their originally uploaded contents.

## Restored original projects and mechanical material

On 2026-10-10, the original uploaded directories were restored from commit `5d6e080df02d82a73eb847e18485b2bdb33146d4`. Their files and original notices are preserved unchanged. These directories include earlier ST/Arm libraries, Wildfire (野火) example material, ZHANGDATOU-related driver/example material and VL53L0X platform files. Applicable terms must be read from each component's supplied license or file-level notice; some VL53L0X platform headers contain separate or proprietary terms. Restoration does not convert these materials to MIT or supply a missing third-party permission.

The MIT license applies to the project author's own code, self-drawn figures and documentation. Supplied third-party models, hardware examples and vendor materials retain their respective terms. A filename or inclusion in this repository does not establish authorship or change the original license.

中文说明：MIT只覆盖本项目作者的原创贡献（包括原创代码、自绘图纸）和发布文档；自动生成、厂商及其他第三方文件保留原版权、原许可证。`firmware/`规范工程中排除旧编译产物、个人调试配置等，不代表原样恢复的旧目录也排除了这些内容。旧目录中的ST/Arm、野火、ZHANGDATOU相关资料及VL53L0X平台文件按各自原许可或文件头声明使用，不能将整个旧目录概括为MIT。第三方随附模型亦不因本次恢复而被重新授权。
