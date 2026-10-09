# Interfaces and text protocol / 接口与文本协议

These assignments and behaviors are read from the retained final sources in [wifi-once](../firmware/wifi-once) and [wifi-retry](../firmware/wifi-retry). They describe the code, not a new electrical or hardware verification. Both variants use the same pin assignments and command set.

## Pin assignments / 接口引脚

| Function / 功能 | STM32 pins / 引脚 | Source setting / 源码设置 |
|---|---|---|
| SDM18 ranging / 测距 | PA2 TX, PA3 RX | USART2, 921600 baud, 8N1 |
| Debug output / 调试输出 | PA9 TX, PA10 RX | USART1, 115200 baud, 8N1; `printf` output |
| ESP8266 AT interface | PB10 TX, PB11 RX | USART3, 115200 baud, 8N1 |
| ESP8266 reset / 复位 | PB9 | Driven low for 100 ms, then high; 2000 ms wait |
| WiFi IO | PB8 | Input with no pull-up/pull-down in the source |
| Stepper enable / 步进使能 | PA5 | EN, set high at initialization |
| Stepper pulse / 步进脉冲 | PA6 | STP, software-generated pin toggles |
| Stepper direction / 步进方向 | PA7 | DIR; high for lowering, low for retrieval |
| SWD programming | PA13, PA14 | SWDIO, SWCLK as recorded in the IOC |

UART TX connects to the other device's RX and vice versa, with a common reference ground. Supply voltages, electrical compatibility, driver enable polarity and mechanical direction must be checked against the actual hardware. No stepper-driver brand is prescribed by these pin definitions. The saved TIM3 setup is not the active pulse-generation method: the final code configures PA6 as a GPIO and toggles it in software.

串口TX/RX交叉连接并共地。实际供电、电平、驱动使能有效电平和机械运动方向需要按实物核对。上述引脚定义不指定步进驱动品牌，也不能把软件延时参数直接解释为电机转速。

## Connection / 网络角色

The ESP8266 is configured as a WiFi station and connects to the TCP server defined in the local `wifi_config.h`. It uses AT commands and transparent TCP transmission. The STM32 firmware is a TCP **client**, not a web server. The original host application is not part of these two source snapshots.

`wifi-once` attempts connection during startup. `wifi-retry` also attempts startup connection and checks for another attempt at a nominal 30 s interval while `wifi_connected` is zero. It does not implement complete detection and recovery of every disconnection after a successful connection; see [Known limitations](KNOWN_LIMITATIONS.md).

ESP8266以STA方式接入现有WiFi，并主动连接本地配置的TCP服务器。两版都不是网页服务端；重试版的重试条件是连接标志为0，不能等同于完整断线恢复。

## Commands / 指令

Send raw ASCII command text, one command per line, terminated by LF or CRLF. Do not wrap it in JSON or add a prefix. CR is ignored and LF completes a command. Commands longer than 100 characters are ignored. Send one command and wait for its response before sending another; the implementation has no command queue.

| Command | Value or action / 参数与行为 |
|---|---|
| `SET_IN_PLACE\|N` | Landed threshold / 到位阈值: 100–1000 mm |
| `SET_GROUND\|N` | Upper/ground threshold / 地面阈值: 500–1500 mm |
| `SET_FULL\|N` | Core-full threshold / 满心阈值: 20–200 mm |
| `SET_STUCK\|N` | Suspected-jamming range / 卡心极差阈值: 1–50 mm |
| `SET_SPEED\|N` | Software delay setting / 软件延时参数: 100–5000, not rpm |
| `GET_PARAMS` | Return current RAM parameters / 返回当前RAM参数 |
| `CMD_DOWN` | Enter manual lowering / 手动下放 |
| `CMD_UP` | Enter manual retrieval / 手动上提 |
| `CMD_STOP` | Request manual stop / 请求手动停止 |
| `CMD_RESET` | Clear movement state and return to automatic mode / 清运动状态并恢复自动模式 |

For example, send `GET_PARAMS` followed by a newline to query settings. Setter values are retained only in RAM. `CMD_RESET` is not an MCU reboot and does not restore default thresholds. Manual movement bypasses the automatic distance-limit path; `CMD_STOP` is a firmware command, not a verified hardware emergency-stop circuit.

参数修改只作用于RAM。`CMD_RESET`不重启MCU、不恢复默认阈值；手动运动不经过自动限位路径。源码中的“急停”命令不能作为硬件安全急停验证。

## State output / 状态报文

The periodic status line has this field order:

```text
DATA|uptime_s|distance_mm|mode|status|in_place|reversing|avg_mm|range_mm|in_place_threshold_mm|ground_threshold_mm|speed_setting
```

| Field | Meaning / 含义 |
|---|---|
| `uptime_s` | `HAL_GetTick()/1000`; elapsed MCU time, not a date/time |
| `distance_mm` | Current cached distance / 当前缓存距离 |
| `mode` | `IDLE`, `DOWN`, `UP`, or `PAUSE` |
| `status` | `IN_PLACE`, `REVERSING`, `DRILLING`, `WAITING`, `PAUSING`, or `NORMAL` |
| `in_place`, `reversing` | State flags / 到位与上提状态标志 |
| `avg_mm` | Integer mean after the buffer fills; otherwise 0 / 缓存充满后为整数均值，否则为0 |
| `range_mm` | Range of all 10 entries, including initial zero entries until filled / 始终计算10项数组极差，未充满时包含初始零值 |
| Last three fields | Current landed/upper thresholds and speed setting / 当前到位、地面阈值及速度设置 |

The parameter response uses:

```text
PARAMS|IN_PLACE=...|GROUND=...|FULL=...|STUCK=...|SPEED=...
```

Additional `OK`, state and alarm messages are emitted, so a host must not assume every line is a `DATA` record. Some final-source message calls use fixed lengths that exceed their literal text length; original behavior is retained and documented in [Known limitations](KNOWN_LIMITATIONS.md).

## Automatic control / 自动流程

The final implementation lowers in the interval between the landed and upper thresholds, stops at the landed threshold, waits 5 s on first landing, and then evaluates core-full followed by suspected-jamming conditions. An alarm triggers a 3 s pause before retrieval to the upper threshold. A zero distance returns early from the control function, and reaching the upper threshold does not automatically start another cycle. Exact execution order is in `Drill_Control`, `Check_Full` and `Check_Stuck` in each variant's `Core/Src/main.c`.

自动路径在到位与地面阈值之间下放，首次到位后等待5 s，再按满心优先、卡心其次的顺序判断；报警后暂停3 s再上提。距离为0时本次控制提前返回，上提结束不自动开始下一轮。阈值条件及顺序以相应版本源码为准。
