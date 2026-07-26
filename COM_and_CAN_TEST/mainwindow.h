#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Windows 特定头文件 - 只在 Windows 下包含
#ifdef Q_OS_WIN
#include <windows.h>
#undef interface  // 解决 Windows 的 interface 宏与 Qt 冲突
#endif

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTextCodec>
#include <QTimer>
#include <QDebug>
#include <QTextBlock>
#include <QMessageBox>
#include <QLineEdit>
#include <QLibrary>
#include <QElapsedTimer>
#include <QThread>
#include <QFileInfo>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QDir>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>



// C 标准头文件 - Linux 下这些都能用
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// 网络测试
#include "rawudpsocket.h"
#include <QUdpSocket>
#include <QNetworkInterface>

// 同星驱动头文件（动态加载，LibUcan2Loader 内部包含 api.h）
#include "libucan2/LibUcan2Loader.h"

// 动态加载的 CAN 适配器
#include "cx/ChuangXinCan.h"
#include "zy/ZhiYuanCan.h"

/* 类型定义 - 建议逐步迁移到 Qt 类型或标准类型 */
#ifndef u8
#define u8 unsigned char
#endif
#ifndef u32
#define u32 unsigned int
#endif

#define FAIL    0
#define SUCC    5
#define MAX_COM_NUM     16
#define COM_RES_TIME 500

// 结构体定义 - C++11 语法 Linux GCC 支持良好
struct
{
    volatile u8 flag = 0;
    volatile u32 num = 0;
} read_wait;

enum CAN_MODU_NAME
{
    NONE = 0,
    TL_MCANFD = 1,    // 同星
    CX_USBCAN = 2,    // 创芯
    ZY_USBCAN = 3     // 致远
};


extern u8 can_modu_name;

// CAN 测试行结构
struct CanTestRow {
    QString ifaceName;
    QPushButton *btnTest;
    QLineEdit *display;
};

// 网络测试行结构
struct NetTestRow {
    QString ifaceName;
    QString ipAddress;
    QPushButton *btnTest;           // 连通测试按钮
    QPushButton *btnBwTest;         // 带宽测试按钮
    QLineEdit *display;             // 连通测试结果
    QLineEdit *latencyDisplay;      // 隐藏的延迟值
    QLineEdit *bwDisplay;           // 带宽测试结果
    QLineEdit *targetIpInput;       // 每行独立的目标IP
};

// 串口测试状态机
enum CommStep {
    STEP_READY,
    STEP_OK1,
    STEP_OK3
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void initCAN();
    void initHardwareCan();
    void Read_Data();
    void SerialPort_init(int port_num);
    void SerialPort_clear();
    void clear_all_line(void);
    void clear_com_line(void);
    u32 comm_test_begin();
    void on_scan_com_btn_clicked();
    void on_tl_begin_test_btn_clicked();
    void com_btn_setEnabled(int con);
    void on_com_test_1_clicked();
    void on_com_test_2_clicked();
    void on_com_test_3_clicked();
    void on_com_test_4_clicked();
    void on_com_test_5_clicked();
    void on_com_test_6_clicked();
    void on_com_test_7_clicked();
    void on_com_test_8_clicked();
    void on_com_test_9_clicked();
    void on_com_test_10_clicked();
    void on_com_test_11_clicked();
    void on_com_test_12_clicked();
    void on_com_test_13_clicked();
    void on_com_test_14_clicked();
    void on_com_test_15_clicked();
    void on_com_test_16_clicked();
    void testSinglePort(int portNum);
    void scanPortsTo();
    void onCanTestButtonClicked();
    QStringList getAvailableCanInterfaces();
    void initDynamicCanTests();
    void autoConfigCanInterface(const QString &ifaceName, int bitrate);
    void performCanHandshake(const QString &ifaceName, QLineEdit *display);

    // 网络测试
    void onNetTestButtonClicked();
    void onNetBwTestButtonClicked();
    void performNetHandshake(const QString &ifaceName, const QString &srcIp,
                             const QString &targetIp, quint16 targetPort,
                             QLineEdit *display, QLineEdit *latencyDisplay);
    void performNetBwTest(const QString &ifaceName, const QString &targetIp,
                          quint16 targetPort, QLineEdit *bwDisplay);
    QStringList getAvailableNetInterfaces();
    void initDynamicNetTests();
    void onNetScanClicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial = nullptr;

    // 串口测试状态机
    CommStep m_commStep = STEP_READY;
    QByteArray m_recvBuf;

    // CAN 动态测试
    QList<CanTestRow> m_canRows;
    u32 portCountLeft = 0;
    u32 portCountRight = 0;

    // 网络测试
    QList<NetTestRow> m_netRows;

    // 创芯动态加载
    ChuangXinCanAdapter *cxCan = nullptr;
    unsigned int cxDevType = 4;      // USBCAN2 设备类型通常为 4
    unsigned int cxDevIndex = 0;
    unsigned int cxCanIndex = 0;

    // 致远动态加载
    ZhiYuanCanAdapter *zyCan = nullptr;
    unsigned int zyDevType = 4;      // 致远 USBCAN2 也可能为 4，但需确认
    unsigned int zyDevIndex = 0;
    unsigned int zyCanIndex = 0;

    // 同星驱动（静态链接）
    // Linux 下需要确认是否有对应库文件
    void* tsDeviceHandle = nullptr;  // 如果用指针保存句柄，建议改为 void* 而不是 DWORD
};
#endif // MAINWINDOW_H
