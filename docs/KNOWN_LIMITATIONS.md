# Implementation notes / 已知实现限制

This release preserves the author-designated final control implementation. These observations come from source inspection, not additional hardware or network experiments. They are documented so that users can reproduce the snapshot and identify what needs validation before extending it. Fixes should be published under a separately identified version rather than silently altering this research snapshot.

本发布保留最终控制实现。以下说明来自源码核对，不是新增实物或网络测试结果；用于解释复现边界。后续修复应另列版本，避免改变已有研究快照。

The relevant implementations are in [wifi-once/main.c](../firmware/wifi-once/Core/Src/main.c), [wifi-retry/main.c](../firmware/wifi-retry/Core/Src/main.c), and each variant's `HARDWARE/SMD18.c`.

| Topic | Actual implementation and implication / 实际实现与含义 |
|---|---|
| Connection retries / 连接重试 | Retry is conditioned on a zero connection flag. There is no complete handling of `CLOSED` notifications or resetting the flag after every send failure. It is failed-connection retry, not guaranteed recovery after any disconnect. / 按连接标志进行失败建连重试，未实现任意断线后的完整恢复。 |
| Timing and buffer / 时序与缓存 | The main loop reads a shared distance cache at a nominal 100 ms interval. AT waits, serial output and the 5 s landed delay can block it. Ten entries do not establish ten independent sensor updates or a fixed 1 s response bound. / 10项为控制程序读取的缓存，不能据此推出独立采样次数或固定响应时间。 |
| Manual movement / 手动运动 | The manual path returns before automatic limit checks. It does not inherit automatic upper/landed stopping protection. / 手动路径绕过自动距离限位。 |
| Zero distance and stopping / 零读数与停止 | A zero distance returns before the manual-control branch, without explicitly stopping the existing motor state. This can delay applying a stop command. / 零读数提前返回，未主动停止原电机状态，也可能延迟停止指令执行。 |
| Transmission lengths / 发送长度 | Several fixed `WiFi_SendData` lengths exceed their string literal payloads. For example, `OK\|IN_PLACE_CHANGED\r\n` is 21 ASCII bytes, but the code passes 23. This can transmit a NUL and, in some calls, read beyond the literal. / 部分固定发送长度超出字符串长度，可能带出NUL或越界字节。 |
| Command buffering / 命令缓存 | The UART interrupt reuses a buffer after setting the command-ready flag; there is no queue or atomic snapshot. Rapid commands can overwrite pending text. Parsing uses `atoi` and does not fully validate integer syntax or relationships among thresholds. / 无命令队列和原子快照，连续指令可能覆盖待处理内容。 |
| Sensor frame bounds / 传感器报文边界 | The 30-byte decoder buffer is indexed using a received length without first checking a valid maximum. CRC validation alone does not protect the preceding indexing path from an abnormal frame. / 缓冲区长度30字节，未先限制报文长度；异常报文可能导致越界。 |
| Pulse speed / 脉冲速度 | PA6 is toggled in the main loop after a delay loop. Timing depends on loop/communication load; the speed setting is not calibrated rpm, and the toggle count is not a measured displacement. / 软件翻转受主循环负载影响，延时参数不是标定转速。 |
| Persistence and access / 持久化与访问 | Parameters are in RAM; there is no saved-parameter path, TLS or command authentication in these files. / 参数不持久保存，当前文件未实现TLS和指令鉴权。 |
| Original project settings / 原工程设置 | RC/xE device/startup records coexist with legacy C8 Flash/SVD references and historical memory regions. Build/download settings must be checked for the actual MCU. / RC/xE记录与C8遗留配置并存，需按实物核对。 |
| Character encoding / 字符编码 | Rebuilding with the saved ARMCC version reports multibyte warnings for Chinese/Unicode `printf` literals, plus missing-final-newline warnings. Actual serial rendering has not been newly tested. / 原工具链对中文、Unicode输出字符串和末尾换行产生警告，未重新验证串口字符显示。 |

No new sensor accuracy, classification accuracy, drilling performance, field reliability or space-qualification result is inferred from this source release.
