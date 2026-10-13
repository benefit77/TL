#!/bin/bash
# 光源控制 —— 平时只需要运行这一个脚本，然后按数字选。
#
# 用法:
#   ./运行.sh              打开菜单
#   ./运行.sh /dev/ttyUSB0 直接连这个串口打开图形界面
#
# 程序在 程序/ 里；想重新编译就先跑 ./编译.sh

cd "$(dirname "$0")" || exit 1

# 程序就在本目录，库在 ./libledim/（程序会自己找到它）
GUI="./libledim-gui"
CLI="./libledim-clidemo"

say()  { printf '%s\n' "$1"; }
warn() { printf '\033[33m%s\033[0m\n' "$1"; }

if [ ! -x "$GUI" ] || [ ! -x "$CLI" ]; then
	say "没有找到可执行文件（libledim-gui / libledim-clidemo）。"
	say "请先运行 ./编译.sh 编译，或把整个文件夹重新拷一份。"
	exit 1
fi

check_permission() {
	if ! id -nG 2>/dev/null | tr ' ' '\n' | grep -qx dialout; then
		warn "提醒: 当前账号不在 dialout 组，接真实灯控板会报 “os error 13”。"
		warn "      解决: sudo usermod -aG dialout $USER   然后注销重登一次。"
		return 1
	fi
	return 0
}

permission_help() {
	say "为什么连不上灯控板（权限说明）"
	say ""
	say "Linux 上串口设备属于 root:dialout，普通账号默认没有读写权限，"
	say "所以打开串口会失败，错误码是 13 (EACCES)。"
	say ""
	say "解决办法:"
	say "  sudo usermod -aG dialout $USER"
	say "  然后注销重新登录（或者执行 newgrp dialout）"
	say ""
	say "验证: 执行下面这条命令，能看到 dialout 就对了"
	say "  id -nG"
	say ""
	say "如果灯控板是 USB 转串口，插上以后串口名一般是 /dev/ttyUSB0。"
}

list_ports() {
	say "本机串口:"
	"$CLI" list
}

open_gui() {
	check_permission
	if [ -n "$1" ]; then
		say "打开图形界面，连接 $1 ..."
		"$GUI" -p "$1"
	else
		say "打开图形界面..."
		say "（界面里点“搜索控制器”，选中串口，再点“连接控制器”）"
		"$GUI"
	fi
}

selftest() {
	check_permission
	local port="$1"
	if [ -z "$port" ]; then
		read -r -p "请输入串口名（例如 /dev/ttyUSB0，可用菜单 2 查看）: " port
	fi
	if [ -z "$port" ]; then
		say "没有输入串口名，返回菜单。"
		return
	fi
	say "开始全通道自检: $port"
	say "（4 个通道依次开关、扫亮度、闪几次）"
	say ""
	"$CLI" -p "$port" selftest
}

compile() {
	"./编译.sh"
}

# 带参数启动时，直接打开界面并连接该串口
if [ $# -ge 1 ]; then
	open_gui "$1"
	exit $?
fi

while true; do
	echo
	echo "================ 光源控制 ================"
	echo "  1) 打开图形界面          （点点鼠标就能控制灯的亮度/开关/频闪）"
	echo "  2) 列出本机串口"
	echo "  3) 全通道自检            （需要接灯控板）"
	echo "  4) 连不上？串口权限说明"
	echo "  5) 重新编译              （改了源码，或在别的机器上第一次用时）"
	echo "  0) 退出"
	read -r -p "请选择 [1]: " choice
	choice=${choice:-1}

	case "$choice" in
	1) open_gui ;;
	2) list_ports ;;
	3) selftest ;;
	4) permission_help ;;
	5) compile ;;
	0) exit 0 ;;
	*) say "请输入 0 ~ 5 之间的数字。" ;;
	esac
done
