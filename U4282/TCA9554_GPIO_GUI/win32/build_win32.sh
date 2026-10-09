#!/bin/sh
#------------------------------------------------------------------------------
# 在 Linux 上交叉编译 Windows (64 位) 版本所需的最小工具链:
#     sudo apt-get install gcc-mingw-w64-x86-64-posix binutils-mingw-w64-x86-64
# 然后在 Windows 上把 SvApiLibx64.dll 放到 exe 同目录即可运行。
#------------------------------------------------------------------------------
set -e

CC="${CC:-x86_64-w64-mingw32-gcc-posix}"
WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"

CFLAGS="-O2 -finput-charset=UTF-8 -fexec-charset=UTF-8 -fwide-exec-charset=UTF-16LE"
LDFLAGS="-mwindows -lcomctl32"

echo "CC = $CC"
"$WINDRES" app.rc -O coff -o app_res.o
"$CC" $CFLAGS -o TCA9554_GPIO_GUI.exe gpio_gui_win32.c app_res.o $LDFLAGS
echo "built: TCA9554_GPIO_GUI.exe"
