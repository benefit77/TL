#ifndef CHUANGXINCAN_H
#define CHUANGXINCAN_H

#include <QLibrary>
#include <QString>
#include <QDebug>
#include <QCoreApplication>
#include <QByteArray>
#include <QObject>
#include <cstring>
#include "../libpathhelper.h"

#ifdef Q_OS_WIN
#include <windows.h>
#undef interface  // 解决 Windows 宏冲突
#else
#define __stdcall  // Linux 下为空定义
#endif

namespace ChuangXin {
    // 创芯结构体（与 ControlCAN.h 一致，但隔离命名空间）
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

    struct VCI_BOARD_INFO {
        unsigned short hw_Version;
        unsigned short fw_Version;
        unsigned short dr_Version;
        unsigned short in_Version;
        unsigned short irq_Num;
        unsigned char can_Num;
        char str_Serial_Num[20];
        char str_hw_Type[40];
        unsigned short Reserved[4];
    };

    // 函数指针定义
    typedef unsigned int (__stdcall *OpenDeviceFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *CloseDeviceFunc)(unsigned int, unsigned int);
    typedef unsigned int (__stdcall *InitCANFunc)(unsigned int, unsigned int, unsigned int, VCI_INIT_CONFIG*);
    typedef unsigned int (__stdcall *StartCANFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *ResetCANFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *TransmitFunc)(unsigned int, unsigned int, unsigned int, VCI_CAN_OBJ*, unsigned int);
    typedef unsigned int (__stdcall *ReceiveFunc)(unsigned int, unsigned int, unsigned int, VCI_CAN_OBJ*, unsigned int, int);
    typedef unsigned int (__stdcall *ClearBufferFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *GetReceiveNumFunc)(unsigned int, unsigned int, unsigned int);
    typedef unsigned int (__stdcall *ReadBoardInfoFunc)(unsigned int, unsigned int, VCI_BOARD_INFO*);
}

class ChuangXinCanAdapter : public QObject {
    Q_OBJECT
public:
    explicit ChuangXinCanAdapter(QObject *parent = nullptr)
        : QObject(parent), m_lib(nullptr) {}

    ~ChuangXinCanAdapter() {
        unload();
    }

    bool load(const QString &dllPath = QString()) {
        QString path = dllPath;
        if (path.isEmpty()) {
#ifdef Q_OS_WIN
            path = findLibFile("ControlCAN.dll", "cx");
#else
            path = findLibFile("libcontrolcan.so", "cx");
#endif
            if (path.isEmpty()) {
                // 兼容旧路径
#ifdef Q_OS_WIN
                path = QCoreApplication::applicationDirPath() + "/cx/ControlCAN.dll";
#else
                path = QCoreApplication::applicationDirPath() + "/cx/libcontrolcan.so";
#endif
            }
        }

        m_lib = new QLibrary(path, this);
        if (!m_lib->load()) {
            qDebug() << "创芯驱动加载失败:" << m_lib->errorString();
            qDebug() << "尝试路径:" << path;
            return false;
        }

        // 解析函数
        m_open = (ChuangXin::OpenDeviceFunc)m_lib->resolve("VCI_OpenDevice");
        m_close = (ChuangXin::CloseDeviceFunc)m_lib->resolve("VCI_CloseDevice");
        m_init = (ChuangXin::InitCANFunc)m_lib->resolve("VCI_InitCAN");
        m_start = (ChuangXin::StartCANFunc)m_lib->resolve("VCI_StartCAN");
        m_reset = (ChuangXin::ResetCANFunc)m_lib->resolve("VCI_ResetCAN");
        m_send = (ChuangXin::TransmitFunc)m_lib->resolve("VCI_Transmit");
        m_recv = (ChuangXin::ReceiveFunc)m_lib->resolve("VCI_Receive");
        m_clear = (ChuangXin::ClearBufferFunc)m_lib->resolve("VCI_ClearBuffer");
        m_getNum = (ChuangXin::GetReceiveNumFunc)m_lib->resolve("VCI_GetReceiveNum");
        m_readInfo = (ChuangXin::ReadBoardInfoFunc)m_lib->resolve("VCI_ReadBoardInfo");

        if (!m_open || !m_send || !m_recv) {
            qDebug() << "创芯关键函数解析失败";
            unload();
            return false;
        }
        return true;
    }

    void unload() {
        if (m_lib) {
            if (m_lib->isLoaded()) m_lib->unload();
            delete m_lib;
            m_lib = nullptr;
        }
    }

    bool isLoaded() const { return m_lib && m_lib->isLoaded(); }

    bool openDevice(unsigned int devType, unsigned int devIndex) {
        return m_open ? (m_open(devType, devIndex, 0) == 1) : false;
    }

    bool closeDevice(unsigned int devType, unsigned int devIndex) {
        return m_close ? (m_close(devType, devIndex) == 1) : false;
    }

    bool initCAN(unsigned int devType, unsigned int devIndex, unsigned int canIndex, unsigned int baudKbps) {
        if (!m_init) return false;
        ChuangXin::VCI_INIT_CONFIG cfg;
        cfg.AccCode = 0;
        cfg.AccMask = 0xFFFFFFFF;
        cfg.Reserved = 0;
        cfg.Filter = 1;
        cfg.Mode = 0;

        // 波特率定时参数（与创芯原版一致）
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
        return m_init(devType, devIndex, canIndex, &cfg) == 1;
    }

    bool startCAN(unsigned int devType, unsigned int devIndex, unsigned int canIndex) {
        return m_start ? (m_start(devType, devIndex, canIndex) == 1) : false;
    }

    bool resetCAN(unsigned int devType, unsigned int devIndex, unsigned int canIndex) {
        return m_reset ? (m_reset(devType, devIndex, canIndex) == 1) : false;
    }

    bool sendFrame(unsigned int devType, unsigned int devIndex, unsigned int canIndex,
                   unsigned int id, const QByteArray &data, bool ext = false, bool remote = false) {
        if (!m_send || data.size() > 8) return false;
        ChuangXin::VCI_CAN_OBJ obj = {0};
        obj.ID = id;
        obj.ExternFlag = ext ? 1 : 0;
        obj.RemoteFlag = remote ? 1 : 0;
        obj.DataLen = data.size();
        memcpy(obj.Data, data.constData(), data.size());
        return m_send(devType, devIndex, canIndex, &obj, 1) == 1;
    }

    // waitTime: 0非阻塞, -1永久等待, >0阻塞毫秒
    int receive(unsigned int devType, unsigned int devIndex, unsigned int canIndex,
                ChuangXin::VCI_CAN_OBJ *buf, unsigned int maxLen, int waitTime = 0) {
        return m_recv ? m_recv(devType, devIndex, canIndex, buf, maxLen, waitTime) : 0;
    }

    bool clearBuffer(unsigned int devType, unsigned int devIndex, unsigned int canIndex) {
        return m_clear ? (m_clear(devType, devIndex, canIndex) == 1) : false;
    }

    unsigned int getReceiveNum(unsigned int devType, unsigned int devIndex, unsigned int canIndex) {
        return m_getNum ? m_getNum(devType, devIndex, canIndex) : 0;
    }

    bool readBoardInfo(unsigned int devType, unsigned int devIndex, ChuangXin::VCI_BOARD_INFO *info) {
        return m_readInfo ? (m_readInfo(devType, devIndex, info) == 1) : false;
    }

private:
    QLibrary *m_lib;
    ChuangXin::OpenDeviceFunc m_open = nullptr;
    ChuangXin::CloseDeviceFunc m_close = nullptr;
    ChuangXin::InitCANFunc m_init = nullptr;
    ChuangXin::StartCANFunc m_start = nullptr;
    ChuangXin::ResetCANFunc m_reset = nullptr;
    ChuangXin::TransmitFunc m_send = nullptr;
    ChuangXin::ReceiveFunc m_recv = nullptr;
    ChuangXin::ClearBufferFunc m_clear = nullptr;
    ChuangXin::GetReceiveNumFunc m_getNum = nullptr;
    ChuangXin::ReadBoardInfoFunc m_readInfo = nullptr;
};

#endif
