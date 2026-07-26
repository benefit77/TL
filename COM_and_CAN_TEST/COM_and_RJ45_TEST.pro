QT       += core gui
QT       += serialport
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# ⚠️ 重要：不要添加 -static、-static-libgcc、-static-libstdc++ 等静态链接标志！
# 这些标志会使 EXE 使用静态 MinGW 运行时，而 Qt DLL 使用动态运行时，
# 两个不同的运行时堆管理器会导致堆损坏 (HEAP_CORRUPTION)，程序启动后几秒崩溃。
# 使用 windeployqt 部署依赖的 DLL 即可（会自动包含 libgcc_s_seh-1.dll 等）。

DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# 只保留实际需要的源文件（删除 canthread.cpp）
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    libucan2/LibUcan2Loader.cpp \
    rawudpsocket.cpp

HEADERS += \
    cx/ChuangXinCan.h \      # 创芯动态加载头文件（必须）
    zy/ZhiYuanCan.h \        # 致远动态加载头文件（必须）
    libucan2/api.h \         # 同星头文件
    libucan2/LibUcan2Loader.h \
    libpathhelper.h \
    mainwindow.h \
    rawudpsocket.h

# 删除或注释掉以下重复/无用的：
# cx/CanLoader.h \
# cx/ControlCAN.h \          # 不再需要，结构体在 ChuangXinCan.h 里
# cx/canthread.h \           # 不再需要
# zy/ControlCAN.h            # 不再需要，结构体在 ZhiYuanCan.h 里

FORMS += \
    mainwindow.ui

RC_ICONS = logo.ico

# libusb（同星 CAN 在 Linux 上需要）
unix:!android {
    LIBS += -lusb-1.0
}

# Winsock（rawudpsocket 在 Windows 上需要）
win32 {
    LIBS += -lws2_32
}
# LIBS += -L$$PWD/cx -lControlCAN    # 保持屏蔽
# LIBS += -L$$PWD/zy -lControlCAN    # 保持屏蔽

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
