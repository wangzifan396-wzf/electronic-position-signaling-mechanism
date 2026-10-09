# Third-party licenses and retained notices

The root `LICENSE` applies to the project author's original contributions and release documentation. It does not relicense generated/vendor code or remove any original copyright notice. Files with their own notice remain governed by that notice; original notices have been retained unchanged.

| Retained component | Applicable original license/notice | Included location in each variant |
|---|---|---|
| Arm CMSIS headers | Apache License 2.0 | `Drivers/CMSIS/LICENSE.txt`, `Drivers/CMSIS/Include/` |
| ST/Arm STM32F1 CMSIS Device | Apache License 2.0 as specified by its supplied license | `Drivers/CMSIS/Device/ST/STM32F1xx/LICENSE.txt` |
| ST STM32F1 HAL/LL driver | BSD-3-Clause when received without a package license, as specified by its supplied license | `Drivers/STM32F1xx_HAL_Driver/LICENSE.txt`; full BSD text also in `LICENSES/ST-BSD-3-Clause.txt` |
| ST generated configuration and startup scaffolding | Original file-level ST notices, including BSD terms where stated | `Core/`, `MDK-ARM/startup_stm32f103xe.s` |

The official [STM32CubeF1 component license table](https://github.com/STMicroelectronics/STM32CubeF1/blob/master/LICENSE.md) lists CMSIS and CMSIS Device under Apache-2.0 and STM32F1 HAL under BSD-3-Clause. The added full BSD text was retrieved from the [official HAL driver license](https://github.com/STMicroelectronics/stm32f1xx-hal-driver/blob/master/LICENSE.md); the original bundled license files are retained as well.

No additional author or license has been invented for helper files that contain no separate notice. Toolchain executables, proprietary device packs, original binaries, and personal debugger configuration are not redistributed.

中文说明：MIT只覆盖本项目作者的原创贡献和发布文档；自动生成及第三方文件保留原版权、原许可证。项目使用开源组件不意味着这些组件已经改为MIT。
