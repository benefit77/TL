#ifndef ZHIYUANCAN_H
#define ZHIYUANCAN_H

#include <QObject>        // 显式包含 QObject
#include <QLibrary>
#include <QString>
#include <QDebug>
#include <QCoreApplication>
#include <QByteArray>
#include <cstring>        // Linux 下 memcpy 需要这个
#include "../libpathhelper.h"

#ifdef Q_OS_WIN
#include <windows.h>
#undef interface
#else
#define __stdcall
#endif

namespace ZhiYuan {
    struct VCI_CAN_OBJ {
        unsigned int ID;
        unsigned int TimeStamp;
        unsigned char TimeFlag;
        unsigned char SendType;
        unsigned char RemoteFlag;
        unsigned char ExternFlag;
        unsigned char DataLen;
        unsigned char Data[8];
        unsigned char Reserved[3];
    };

    struct VCI_INIT_CONFIG {
        unsigned int AccCode;
        unsigned int AccMask;
        unsigned int Reserved;
        unsigned char Filter;
        unsigned char Timing0;
        unsigned char Timing1;
        unsigned char Mode;
    };

    typedef unsigned int (__stdcall *OpenDeviceFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *CloseDeviceFunc)(unsigned int, unsigned int);
    typedef unsigned int (__stdcall *InitCANFunc)(unsigned int, unsigned int, unsigned int, VCI_INIT_CONFIG*);
    typedef unsigned int (__stdcall *StartCANFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *TransmitFunc)(unsigned int, unsigned int, unsigned int, VCI_CAN_OBJ*, unsigned int);
    typedef unsigned int (__stdcall *ReceiveFunc)(unsigned int, unsigned int, unsigned int, VCI_CAN_OBJ*, unsigned int, int);
    typedef unsigned int (__stdcall *ClearBufferFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *GetReceiveNumFunc)(unsigned int, unsigned int, unsigned int);
}

class ZhiYuanCanAdapter : public QObject {
    Q_OBJECT
public:
    explicit ZhiYuanCanAdapter(QObject *parent = nullptr)
        : QObject(parent), m_lib(nullptr) {}

    ~ZhiYuanCanAdapter() {
        unload();
    }

    bool load(const QString &dllPath = QString()) {
        QString path;
        if (!dllPath.isEmpty()) {
            path = dllPath;
        } else {
#ifdef Q_OS_WIN
            path = findLibFile("ControlCAN.dll", "zy");
#else
            path = findLibFile("libusbcan.so", "zy");
#endif
            if (path.isEmpty()) {
                QString basePath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_WIN
                path = basePath + "/zy/ControlCAN.dll";
#else
                path = basePath + "/zy/libusbcan.so";
#endif
            }
        }

        m_lib = new QLibrary(path, this);
        if (!m_lib->load()) {
            qDebug() << "致远驱动加载失败:" << m_lib->errorString();
            qDebug() << "尝试路径:" << path;
            return false;
        }

        // 解析函数
        m_open = (ZhiYuan::OpenDeviceFunc)m_lib->resolve("VCI_OpenDevice");
        m_close = (ZhiYuan::CloseDeviceFunc)m_lib->resolve("VCI_CloseDevice");
        m_init = (ZhiYuan::InitCANFunc)m_lib->resolve("VCI_InitCAN");
        m_start = (ZhiYuan::StartCANFunc)m_lib->resolve("VCI_StartCAN");
        m_send = (ZhiYuan::TransmitFunc)m_lib->resolve("VCI_Transmit");
        m_recv = (ZhiYuan::ReceiveFunc)m_lib->resolve("VCI_Receive");
        m_clear = (ZhiYuan::ClearBufferFunc)m_lib->resolve("VCI_ClearBuffer");
        m_getNum = (ZhiYuan::GetReceiveNumFunc)m_lib->resolve("VCI_GetReceiveNum");

        if (!m_open || !m_send || !m_recv) {
            qDebug() << "致远关键函数解析失败: open=" << (m_open != nullptr)
                     << "send=" << (m_send != nullptr)
                     << "recv=" << (m_recv != nullptr);
            unload();
            return false;
        }
        return true;
    }

    void unload() {
        if (m_lib) {
            if (m_lib->isLoaded()) {
                m_lib->unload();
            }
            delete m_lib;
            m_lib = nullptr;
        }
    }

    bool isLoaded() const {
        return m_lib && m_lib->isLoaded();
    }

    bool openDevice(unsigned int type, unsigned int idx) {
        return m_open ? (m_open(type, idx, 0) == 1) : false;
    }

    bool closeDevice(unsigned int type, unsigned int idx) {
        return m_close ? (m_close(type, idx) == 1) : false;
    }

    bool initCAN(unsigned int type, unsigned int devIdx, unsigned int canIdx, unsigned int baudKbps) {
        if (!m_init) return false;
        ZhiYuan::VCI_INIT_CONFIG cfg = {};
        cfg.AccCode = 0;
        cfg.AccMask = 0xFFFFFFFF;
        cfg.Filter = 1;
        cfg.Mode = 0;

        switch(baudKbps) {
            case 1000: cfg.Timing0 = 0x00; cfg.Timing1 = 0x14; break;
            case 800:  cfg.Timing0 = 0x00; cfg.Timing1 = 0x16; break;
            case 500:  cfg.Timing0 = 0x00; cfg.Timing1 = 0x1C; break;
            case 250:  cfg.Timing0 = 0x01; cfg.Timing1 = 0x1C; break;
            case 125:  cfg.Timing0 = 0x03; cfg.Timing1 = 0x1C; break;
            case 100:  cfg.Timing0 = 0x04; cfg.Timing1 = 0x1C; break;
            case 50:   cfg.Timing0 = 0x09; cfg.Timing1 = 0x1C; break;
            default:   cfg.Timing0 = 0x00; cfg.Timing1 = 0x1C; break;
        }
        return m_init(type, devIdx, canIdx, &cfg) == 1;
    }

    bool startCAN(unsigned int type, unsigned int devIdx, unsigned int canIdx) {
        return m_start ? (m_start(type, devIdx, canIdx) == 1) : false;
    }

    bool sendFrame(unsigned int type, unsigned int devIdx, unsigned int canIdx,
                   unsigned int id, const QByteArray &data, bool ext = false, bool remote = false) {
        if (!m_send || data.size() > 8) return false;
        ZhiYuan::VCI_CAN_OBJ obj = {};
        obj.ID = id;
        obj.ExternFlag = ext ? 1 : 0;
        obj.RemoteFlag = remote ? 1 : 0;
        obj.DataLen = data.size();
        memcpy(obj.Data, data.constData(), data.size());
        return m_send(type, devIdx, canIdx, &obj, 1) == 1;
    }

    int receive(unsigned int type, unsigned int devIdx, unsigned int canIdx,
                ZhiYuan::VCI_CAN_OBJ *buf, unsigned int maxLen, int waitTime = 0) {
        return m_recv ? m_recv(type, devIdx, canIdx, buf, maxLen, waitTime) : 0;
    }

    bool clearBuffer(unsigned int type, unsigned int devIdx, unsigned int canIdx) {
        return m_clear ? (m_clear(type, devIdx, canIdx) == 1) : false;
    }

    unsigned int getReceiveNum(unsigned int type, unsigned int devIdx, unsigned int canIdx) {
        return m_getNum ? m_getNum(type, devIdx, canIdx) : 0;
    }

private:
    QLibrary *m_lib;
    ZhiYuan::OpenDeviceFunc m_open = nullptr;
    ZhiYuan::CloseDeviceFunc m_close = nullptr;
    ZhiYuan::InitCANFunc m_init = nullptr;
    ZhiYuan::StartCANFunc m_start = nullptr;
    ZhiYuan::TransmitFunc m_send = nullptr;
    ZhiYuan::ReceiveFunc m_recv = nullptr;
    ZhiYuan::ClearBufferFunc m_clear = nullptr;
    ZhiYuan::GetReceiveNumFunc m_getNum = nullptr;
};

#endif
