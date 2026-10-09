QT       += core gui widgets
CONFIG   += c++11
TEMPLATE  = app
TARGET    = TCA9554_GPIO_GUI

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tca9554.cpp

HEADERS += \
    mainwindow.h \
    tca9554.h

#-----------------------------------------------------------------------------
# Vendor library "SvApiLib" (Seavo / 信步 TCA_GPIO_RW)
# Override with:  qmake VENDOR_DIR=/path/to/TCA_GPIO_RW
#-----------------------------------------------------------------------------
isEmpty(VENDOR_DIR) {
    VENDOR_DIR = $$clean_path($$PWD/../TCA_GPIO_RW/TCA_GPIO_RW)
}

unix {
    INCLUDEPATH += $$VENDOR_DIR
    LIBS        += $$VENDOR_DIR/SvApiLib.a
    # SvApiLib.a is not position independent -> build a non-PIE executable
    QMAKE_LFLAGS += -no-pie
}

win32 {
    # SvApiLibx64.dll is loaded at run time (see tca9554.cpp),
    # just make sure the dll is next to the executable.
    INCLUDEPATH += $$VENDOR_DIR
}
