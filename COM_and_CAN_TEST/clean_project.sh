#!/bin/bash
# ============================================================
# 清理项目中不需要的文件（在 Ubuntu 上运行）
# ============================================================
echo "清理不需要的文件..."

# Qt Creator 用户配置（每台机器不同）
rm -f *.pro.user*

# Windows 编译输出
rm -rf release/ build/
rm -f *.exe *.dll *.a *.def

# Windows 桩文件（Linux 上用不到）
rm -f libucan2/libucan2_stub_win.cpp

# 不需要的同星辅助头文件（api.h 已包含所需内容）
rm -f libucan2/comer.h
rm -f libucan2/Comproto.h
rm -f libucan2/dev2_api.h
rm -f libucan2/internal.h
rm -f libucan2/libusb.h    # 系统自带，不需要项目提供
rm -f libucan2/magic.h

# 旧的 ControlCAN 头文件（已被 ChuangXinCan.h / ZhiYuanCan.h 替代）
rm -f cx/controlcan.h
rm -f zy/controlcan.h

# Qt 翻译文件（未使用）
rm -f COM_and_RJ45_TEST_zh_CN.ts

echo "清理完成！"
echo ""
echo "保留的文件:"
echo "  main.cpp  mainwindow.cpp  mainwindow.h  mainwindow.ui"
echo "  COM_and_RJ45_TEST.pro"
echo "  libpathhelper.h"
echo "  pack_appimage.sh"
echo "  cx/ChuangXinCan.h"
echo "  zy/ZhiYuanCan.h"
echo "  libucan2/api.h  libucan2/LibUcan2Loader.h/.cpp"
echo "  libucan2/libucan2.so  libucan2/libucan2_dev2.so"
