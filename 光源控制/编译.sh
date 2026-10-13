#!/bin/bash
# 一键编译：编译 源码/，并把编好的程序更新到 程序/ 里。
# 在别的机器上第一次用，先跑这个。

cd "$(dirname "$0")" || exit 1

if ! command -v cmake >/dev/null 2>&1; then
	echo "没找到 cmake。请先执行:  sudo apt install build-essential cmake"
	exit 1
fi

# 整个文件夹拷到别的机器/别的目录时，build/ 里记着原来那个绝对路径，CMake 会拒绝复用。
# 这里先判断一下，是别人的缓存就换个目录编，不动你已经编好的东西。
BUILD_DIR=build
if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
	cached_source=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$BUILD_DIR/CMakeCache.txt" | head -1)
	if [ -n "$cached_source" ] && [ "$cached_source" != "$PWD" ]; then
		echo "提示: build/ 是在别的位置编的（$cached_source），本次改用 build-new/"
		BUILD_DIR=build-new
	fi
fi

echo "=== 1/3 配置 ==="
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release || exit 1

echo
echo "=== 2/3 编译 ==="
cmake --build "$BUILD_DIR" -j"$(nproc 2>/dev/null || echo 2)" || exit 1

echo
echo "=== 3/3 归位 ==="
# the build tree is laid out the same way this project is (programs on top, library in libledim/),
# so this is a straight copy rather than a reshuffle
cp -a "$BUILD_DIR/libledim-gui" "$BUILD_DIR/libledim-clidemo" . 2>/dev/null
cp -a "$BUILD_DIR/libledim/libledim.so.2.0.0" "$BUILD_DIR/libledim/libledim.so.2" "$BUILD_DIR/libledim/libledim.so" libledim/

echo "项目根目录:"
ls -1 libledim-gui libledim-clidemo 2>/dev/null
echo "库文件夹 libledim/:"
ls -1 libledim/libledim.so* 2>/dev/null
echo "头文件 libledim/include/:"
ls -1 libledim/include/ 2>/dev/null

if [ ! -e libledim-gui ]; then
	echo
	echo "注意: 没有编出图形界面，说明系统里没有 Qt5。"
	echo "      需要界面就执行:  sudo apt install qtbase5-dev   然后重跑本脚本。"
	echo "      （库和命令行不受影响，控制光源照样能用）"
fi

echo
echo "=== 下一步 ==="
echo "  ./运行.sh                                    中文菜单"
echo "  ./libledim-clidemo -p /dev/ttyUSB0 selftest  接上灯控板后全通道自检"
echo "  ./libledim-gui -p /dev/ttyUSB0               打开图形界面并连接"
echo
echo "（接真实灯控板前确认账号在 dialout 组: sudo usermod -aG dialout \$USER）"
