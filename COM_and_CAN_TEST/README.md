# COM_and_CAN_TEST

Qt 5 GUI 串口/CAN 总线测试工具。支持串口通信测试、SocketCAN 以及同星/创芯/致远 USB CAN 模块测试。

---

## 环境要求 (Ubuntu Server)

```bash
# 编译依赖
sudo apt install build-essential qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev

# 运行时依赖
sudo apt install libqt5serialport5 libqt5widgets5 libusb-1.0-0

# CAN 工具（可选，用于调试）
sudo apt install can-utils
```

---

## 编译

```bash
cd COM_and_CAN_TEST
qmake COM_and_RJ45_TEST.pro
make -j$(nproc)
```

生成可执行文件 `COM_and_RJ45_TEST`。

---

## 运行

### 方式一：Offscreen 模式（推荐，纯命令行）

```bash
QT_QPA_PLATFORM=offscreen ./COM_and_RJ45_TEST
```

> 程序在后台静默运行，看不到窗口。**但默认需要手动点击按钮才执行测试**，如需自动测试请参见下方 CLI 模式说明。

### 方式二：Xvfb 虚拟显示

```bash
sudo apt install xvfb
xvfb-run ./COM_and_RJ45_TEST
```

---

## CAN 测试配置

程序自动按以下顺序检测 CAN 设备：

```
SocketCAN (can0/can1) → 同星 TL-MCANFD → 创芯 CX_USBCAN → 致远 ZY_USBCAN
```

### 1. SocketCAN（系统原生 CAN 接口）

适用于 SPI CAN 模块（如 MCP2515）、PCIe CAN 卡、或纯软件虚拟 CAN。

```bash
# 加载内核模块
sudo modprobe can
sudo modprobe can_raw
sudo modprobe can_dev

# 配置并启用 CAN 接口
sudo ip link set can0 type can bitrate 500000
sudo ip link set up can0

# 如需 CAN FD
sudo ip link set can0 type can bitrate 500000 dbitrate 2000000 fd on

# 验证
ip -details link show can0
```

> 程序内部 `autoConfigCanInterface()` 会自动执行 `ip link set` 配置（波特率 500k）。

#### 模拟 CAN 对手（回环测试）

需要另一个设备回复 ID=`0x101`、data[0]=轮次(1~10) 的 CAN 帧。可用 `cangen` 模拟：

```bash
# 终端 1：监听
candump can0

# 终端 2：自动回复（模拟 DUT）
cangen can0 -I 0x101 -L 8 -D i1234567 -g 50
```

### 2. 同星 TL-MCANFD（USB）

需要 `libucan2.so` 动态库和 libusb。

```bash
# 安装 libusb
sudo apt install libusb-1.0-0-dev

# 查看 USB 设备
lsusb

# udev 权限（替换 VID 为实际值）
sudo tee /etc/udev/rules.d/99-tlcan.rules << 'EOF'
SUBSYSTEM=="usb", ATTRS{idVendor}=="1d6b", MODE="0666", GROUP="plugdev"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger

# 将 libucan2.so 放在可执行文件同级目录
cp libucan2.so ./
```

### 3. 创芯 CX_USBCAN（USB）

需要 `libcontrolcan.so` 动态库。

```bash
mkdir -p cx
cp libcontrolcan.so ./cx/

# USB 权限
sudo tee /etc/udev/rules.d/99-cxcan.rules << 'EOF'
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="a23c", MODE="0666"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger
```

### 4. 致远 ZY_USBCAN（USB）

需要 `libusbcan.so` 动态库。

```bash
mkdir -p zy
cp libusbcan.so ./zy/

# USB 权限（与创芯类似，根据实际 VID/PID 修改）
sudo tee /etc/udev/rules.d/99-zycan.rules << 'EOF'
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="a23c", MODE="0666"
EOF
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## 串口测试配置

程序自动扫描可用串口，对每个串口执行三步握手协议：

```
发送方 → "YES\r\n"   等待 "OK1"
发送方 → "OK2\r\n"   等待 "OK3"
```

被测设备（DUT）需要运行以下逻辑：

```python
# Python 示例：串口 DUT
import serial, time
ser = serial.Serial('/dev/ttyXXX', 115200)
while True:
    data = ser.read()  # 收到 'Y' 开头
    if b'YES' in data:
        ser.write(b'OK1\r\n')
        time.sleep(0.05)
        ser.write(b'OK3\r\n')
```

---

## 诊断命令

```bash
# CAN 接口状态
ip link show can0
ip -details link show can0
cat /sys/class/net/can0/type

# 调试 CAN 帧
candump can0

# USB 设备
lsusb
lsusb -t

# 查看内核 CAN 日志
dmesg | grep -i can

# 检查串口
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|---|---|---|
| `QLibrary::load()` 失败 | 缺少 `.so` 或路径不对 | 将 `libucan2.so`/`libcontrolcan.so`/`libusbcan.so` 放在可执行文件同级 |
| `libusb` 相关错误 | 未安装 libusb | `sudo apt install libusb-1.0-0-dev` |
| CAN socket 创建失败 | 未加载内核模块 | `sudo modprobe can can_raw` |
| USB 设备无权限 | 缺少 udev 规则 | 参考上方创建 `/etc/udev/rules.d/99-*.rules` |
| `QXcbConnection: Could not connect to display` | 无 X11 显示 | 改用 `QT_QPA_PLATFORM=offscreen` 或 `xvfb-run` |
