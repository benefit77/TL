# TCA9554 GPIO 测试程序 (图形界面)

一个类似厂商 "GPIO和看门狗测试程序" 的 Qt 图形界面工具：**DO 可控制**、**DI 只读**。
通过信步/拓朗提供的 `SvApiLib` 访问 TCA9554 (TCA9554A) 芯片。

## 硬件信息 (来自 GPIO.txt)

| 项目 | 值 |
| --- | --- |
| SMBus 地址 | `0x40` |
| DI1 / DI2 / DI3 / DI4 | bit 7 / 6 / 5 / 4（输入，只读）|
| DO1 / DO2 / DO3 / DO4 | bit 3 / 2 / 1 / 0（输出，可控制）|

寄存器：`0x00` 输入、`0x01` 输出、`0x02` 极性、`0x03` 方向配置。
程序启动时把 DI 设为输入、DO 设为输出，之后 DO 可写、DI 只读。

## 编译

### Linux (Qt5)

需要 `qtbase5-dev`，并把 `SvApiLib.h` / `SvApiLib.a` 放在 `../TCA_GPIO_RW/TCA_GPIO_RW`
（默认路径，也可用 `qmake VENDOR_DIR=...` 指定）。

```bash
cd TCA9554_GPIO_GUI
qmake TCA9554_GPIO_GUI.pro
make
sudo ./TCA9554_GPIO_GUI      # 访问 SMBus 需要 root
```

### Windows (Qt + SvApiLibx64.dll)

用 Qt Creator 打开 `TCA9554_GPIO_GUI.pro` 编译；把 `SvApiLibx64.dll`
（以及厂商驱动）放到可执行文件同一目录，并以管理员身份运行。

## 使用

顶部一行是设备状态（`● 已连接 · SMBus 地址 0x40`，连不上时变红）和最近一次操作。

* **输出 (DO)**：OUT1~OUT4 的当前值显示在方框里，点击方框即取反并写入
  （1 -> 0，0 -> 1）；高电平显示绿色、低电平灰色。
* **输入 (DI)**：IN1~IN4 的当前值同样显示在方框里（只读，不能点击），每 0.5 秒自动刷新。
* **底部**：`IN / OUT / CFG` 三个寄存器的十六进制原始值，方便对照调试；
  `全部输出清零` 一键把 4 路输出归零。

## 连不上时（框里显示 `?`）怎么办

DO 框显示 `?`、左上角是红色「● 未连接」时，就是**程序没能打开设备**；
右上角红字会写明原因，常见几种：

| 提示 | 原因 / 处理 |
| --- | --- |
| `需要 root 权限访问 SMBus` | 用 `sudo ./TCA9554_GPIO_GUI` 运行 |
| `初始化驱动失败` | 用 root 运行，并确认目标机的 SMBus / 厂商驱动可用 |
| `无法加载 SvApiLibx64.dll` | dll 没和 exe 放同一目录，或位数不对（要 64 位）|
| `SvApiLibInitialize() 失败` | 没用管理员身份运行，或驱动 `SvIoCtrlx64.sys` 没装 |

在**开发机（没有 TCA9554 硬件）**上运行必然连不上，这是正常的。

> 说明：本程序只做 GPIO 部分。厂商的 "看门狗测试" 需要额外的看门狗 API
> （现有 Linux `SvApiLib` 未导出），如需可在此基础上补充。
