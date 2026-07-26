# COM_and_CAN_Responder

**DUT（Device Under Test）模拟器** — 与 `COM_and_CAN_TEST` 测试软件配套的响应端工具。

## 功能

本软件模拟被测设备（DUT），自动响应测试端发出的握手协议，用于验证串口和 CAN 总线通信是否正常。

### 🔌 串口响应

与测试端 `COM_and_CAN_TEST` 的串口握手协议对应：

| 方向 | 内容 | 说明 |
|------|------|------|
| DUT → 测试端 | `ready\r\n` | 连接后主动发送就绪信号 |
| 测试端 → DUT | `YES\r\n` | 测试端确认收到 ready |
| DUT → 测试端 | `OK1\r\n` | DUT 确认收到 YES |
| 测试端 → DUT | `OK2\r\n` | 测试端确认收到 OK1 |
| DUT → 测试端 | `OK3\r\n` | DUT 最终确认，握手完成 |

### 🚌 CAN 响应

与测试端 `COM_and_CAN_TEST` 的 CAN 握手协议对应：

| 方向 | CAN ID | 数据 | 说明 |
|------|--------|------|------|
| 测试端 → DUT | `0x100` | `[round, 0x11, 0x12, ...]` | 测试请求，round=1~10 |
| DUT → 测试端 | `0x101` | `[round, 0x11, 0x12, ...]` | DUT 回复，数据与请求一致 |

## 编译

### 环境要求

- Qt 5.15+
- Qt SerialPort 模块
- 编译器: GCC / MinGW / MSVC
- Linux: `build-essential qtbase5-dev libqt5serialport5-dev`

### 编译步骤

```bash
cd COM_and_CAN_Responder
qmake COM_and_CAN_Responder.pro
make -j$(nproc)
```

## 使用说明

1. **启动程序**
   - Windows: 双击 `COM_and_CAN_Responder.exe`
   - Linux: `./COM_and_CAN_Responder`

2. **串口响应**
   - 点击「扫描」检测可用串口
   - 从下拉列表选择串口
   - 点击「打开串口」开始监听
   - 当测试端连接时，程序自动完成握手响应
   - 日志区会显示收发数据

3. **CAN 响应**
   - 点击「扫描」检测可用 CAN 接口
   - 选择接口（SocketCAN 或模拟模式）
   - 点击「打开 CAN」开始监听
   - 勾选「自动响应」自动回复测试帧（默认开启）
   - 日志区会显示收到的 CAN 帧和回复的帧

4. **日志查看**
   - 所有收发数据带有时间戳显示在日志区
   - 可使用「清除日志」按钮清空

## 测试流程

推荐与 `COM_and_CAN_TEST` 配合使用：

```
┌─────────────────────┐          ┌─────────────────────┐
│  COM_and_CAN_TEST   │          │ COM_and_CAN_Respond │
│    (测试端)         │ ◄──────► │    (DUT 模拟器)     │
│                     │  串口/   │                     │
│  发送请求 →         │   CAN    │  ← 自动响应         │
│  验证响应 ←         │          │                      │
└─────────────────────┘          └─────────────────────┘
```
