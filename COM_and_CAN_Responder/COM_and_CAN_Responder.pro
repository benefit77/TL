QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    serialresponder.cpp \
    canresponder.cpp

HEADERS += \
    mainwindow.h \
    serialresponder.h \
    canresponder.h

FORMS += \
    mainwindow.ui

# libusb（Linux 上同星 CAN 需要）
unix:!android {
    LIBS += -lusb-1.0
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
