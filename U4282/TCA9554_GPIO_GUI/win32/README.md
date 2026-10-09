# Windows 版 (原生 Win32, 不依赖 Qt)

一个单文件 `TCA9554_GPIO_GUI.exe`，界面和功能与 Qt 版一致：
**输出 (DO) 可控制、输入 (DI) 只读**，SMBus 地址 `0x40`。

布局预览（近似图，实际以程序为准）：`preview_win32.png`

用原生 Win32 API 写成，**不需要安装 Qt**，只要厂商的 `SvApiLibx64.dll`
（在 `Win_TCA9554_GPIO_RW/.../Release/X64/` 里）即可。

## 运行

1. `TCA9554_GPIO_GUI.exe` 和 `SvApiLibx64.dll` 放同一目录
   （`SvApiLibx64.dll` 已随仓库提供；exe 用 `sh build_win32.sh` 或 `build_win32.bat` 编译）。
2. 确保厂商驱动 `SvIoCtrlx64.sys` 已安装。
3. 双击运行（程序清单要求管理员权限，会自动弹 UAC）。

## 编译

* Linux 交叉编译: `sh build_win32.sh`
  （需要 `gcc-mingw-w64-x86-64-posix` / `binutils-mingw-w64-x86-64`）
* Windows 上用 MinGW: 运行 `build_win32.bat`
* 也可以用 Qt Creator / Qt 自带 MinGW 的 `gcc`、`windres`。

## 说明

顶部一行是设备状态（`● 已连接 · SMBus 地址 0x40`，连不上时变红）和最近一次操作。

* DO: OUT1~OUT4 的当前值显示在方框里，点击方框即取反并写入（1 -> 0，0 -> 1），
  高电平显示绿色、低电平灰色。
* DI: IN1~IN4 的当前值同样显示在方框里（只读，不能点击），每 0.5 秒自动刷新。
* 底部: `IN / OUT / CFG` 寄存器十六进制原始值；`全部输出清零` 一键归零 4 路输出。
* 读写通过 `SvSmbReadByte` / `SvSmbWriteByte` 操作 TCA9554 的
  `0x00`(输入) / `0x01`(输出) / `0x03`(方向) 寄存器。

## 连不上时（框里显示 `?`）怎么办

DO 框显示 `?`、左上角红色「● 未连接」，说明**没能打开设备**；
顶部红字会写明原因：

| 提示 | 原因 / 处理 |
| --- | --- |
| `找不到 SvApiLibx64.dll` | dll 没和 exe 放同一目录 |
| `SvApiLibx64.dll 位数不对` | exe 是 64 位，必须配 64 位的 `SvApiLibx64.dll` |
| `找不到导出函数` | dll 版本不对（缺少 `SvSmbReadByte` 等）|
| `SvApiLibInitialize() 失败` | 没用管理员身份运行，或驱动 `SvIoCtrlx64.sys` 没装 |
