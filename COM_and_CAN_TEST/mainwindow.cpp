// 必须在任何 Windows 头之前包含
#ifdef Q_OS_WIN
#define _WIN32_WINNT 0x0600  // 启用最新的 Windows API 定义
#include <winsock2.h>
#include <windows.h>
#undef interface
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QDateTime>
#include <QFrame>

// Linux CAN 接口头文件
#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#endif


u32 Port_num = 0;

u8 can_modu_name = NONE;


typedef struct {
    uint8_t  ucan2_channel;
    uint32_t baud_rate;
    uint32_t baud_data;
    int      daemon_mode;
    int      verbose;      // 1=打印实时收发到终端
    uint32_t stats_interval;
} gateway_config_t;

static gateway_config_t g_cfg = {
    1,          // ucan2_channel
    500000,     // baud_rate
    2000000,    // baud_data
    0,          // daemon_mode
    0,          // verbose
    10          // stats_interval
};


// ============================================
// 新增：分离出来的 CAN 初始化函数
// ============================================
void MainWindow::initCAN()
{
    // ====== 第一步：尝试系统 SocketCAN 接口 ======
    QStringList canIfaces = getAvailableCanInterfaces();
    if (!canIfaces.isEmpty()) {
        qDebug() << "检测到系统 CAN 接口:" << canIfaces;
        can_modu_name = NONE;  // 使用 SocketCAN，不标记为硬件模块
        initDynamicCanTests();
        return;
    }

    // ====== 第二步：没有 SocketCAN，尝试硬件 CAN 模块 ======
    qDebug() << "未检测到系统 CAN 接口，尝试硬件 CAN 模块...";
    initHardwareCan();
}

void MainWindow::initHardwareCan()
{
    const unsigned int nDeviceType = 4;
    const unsigned int nDeviceInd = 0;
    const unsigned int nCANInd = g_cfg.ucan2_channel - 1;

    bool initSuccess = false;

    // 清理已有资源
    if (cxCan) {
        cxCan->closeDevice(cxDevType, cxDevIndex);
        delete cxCan;
        cxCan = nullptr;
    }
    if (zyCan) {
        zyCan->closeDevice(4, 0);
        delete zyCan;
        zyCan = nullptr;
    }
    can_modu_name = NONE;

    // ========== 1. 尝试 TL_MCANFD（同星）- 动态加载 ==========
    if (LibUcan2Loader::load()) {
        qDebug() << "TL-CANFD 动态库加载成功";
        if (LibUcan2Loader::Init()) {
            qDebug() << "TL-CANFD OPEN DEV SUCC";
            LibUcan2Loader::EndisChannel(g_cfg.ucan2_channel, false);
            LibUcan2Loader::EndisChannel(g_cfg.ucan2_channel + 1, false);
            if (LibUcan2Loader::InitChannel(g_cfg.ucan2_channel, true, g_cfg.baud_rate, g_cfg.baud_data) &&
                    LibUcan2Loader::InitChannel(g_cfg.ucan2_channel + 1, true, g_cfg.baud_rate, g_cfg.baud_data)) {
                qDebug() << "TL-CANFD init SUCC";
                if (LibUcan2Loader::EndisChannel(g_cfg.ucan2_channel, true) &&
                        LibUcan2Loader::EndisChannel(g_cfg.ucan2_channel + 1, true)) {
                    qDebug() << "TL-CANFD OPEN CHANNEL SUCC";
                    can_modu_name = TL_MCANFD;
                    initSuccess = true;
                }
            }
        } else {
            qDebug() << "TL-CANFD Init() 失败 - USB设备未连接或权限不足";
        }
    } else {
        qDebug() << "TL-CANFD 动态库加载失败";
    }

    // ========== 2. 尝试创芯 CX_USBCAN（动态加载）==========
    if (!initSuccess) {
        cxCan = new ChuangXinCanAdapter(this);
        if (cxCan->load()) {
            cxDevType = nDeviceType;
            cxDevIndex = nDeviceInd;
            if (cxCan->openDevice(cxDevType, cxDevIndex)) {
                bool ch0_ok = cxCan->initCAN(cxDevType, cxDevIndex, 0, g_cfg.baud_rate / 1000) &&
                        cxCan->startCAN(cxDevType, cxDevIndex, 0);
                bool ch1_ok = cxCan->initCAN(cxDevType, cxDevIndex, 1, g_cfg.baud_rate / 1000) &&
                        cxCan->startCAN(cxDevType, cxDevIndex, 1);
                if (ch0_ok && ch1_ok) {
                    cxCanIndex = nCANInd;
                    can_modu_name = CX_USBCAN;
                    initSuccess = true;
                    qDebug() << "创芯CAN初始化成功";
                } else {
                    qDebug() << "创芯通道初始化失败";
                    cxCan->closeDevice(cxDevType, cxDevIndex);
                    delete cxCan;
                    cxCan = nullptr;
                }
            } else {
                qDebug() << "创芯OpenDevice失败";
                delete cxCan;
                cxCan = nullptr;
            }
        } else {
            delete cxCan;
            cxCan = nullptr;
        }
    }

    // ========== 3. 尝试致远 ZY_USBCAN（动态加载）==========
    if (!initSuccess) {
        zyCan = new ZhiYuanCanAdapter(this);
        if (zyCan->load()) {
            unsigned int zyDevType = 4;
            unsigned int zyDevIndex = 0;
            if (zyCan->openDevice(zyDevType, zyDevIndex)) {
                bool ch0_ok = zyCan->initCAN(zyDevType, zyDevIndex, 0, g_cfg.baud_rate / 1000) &&
                        zyCan->startCAN(zyDevType, zyDevIndex, 0);
                bool ch1_ok = zyCan->initCAN(zyDevType, zyDevIndex, 1, g_cfg.baud_rate / 1000) &&
                        zyCan->startCAN(zyDevType, zyDevIndex, 1);
                if (ch0_ok && ch1_ok) {
                    zyCanIndex = nCANInd;
                    can_modu_name = ZY_USBCAN;
                    initSuccess = true;
                    qDebug() << "致远CAN初始化成功";
                } else {
                    zyCan->closeDevice(zyDevType, zyDevIndex);
                    delete zyCan;
                    zyCan = nullptr;
                }
            } else {
                delete zyCan;
                zyCan = nullptr;
            }
        } else {
            delete zyCan;
            zyCan = nullptr;
        }
    }

    // ========== 结果处理 ==========
    if (!initSuccess) {
        qCritical() << "所有 CAN 设备初始化失败";
        can_modu_name = NONE;
        initDynamicCanTests();  // 仍创建 UI，显示"未检测到 CAN 接口"
    } else {
        qDebug() << "硬件 CAN 初始化成功，当前模块:" << can_modu_name;
        initDynamicCanTests();  // 创建 UI，显示静态 CAN1/CAN2
    }
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
#ifdef Q_OS_WIN
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
#else
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    ui->setupUi(this);
    clear_all_line();

    // 初始化 CAN 测试界面（先尝试系统 SocketCAN，失败则回退到硬件库）
    initCAN();

    this->show();
}




MainWindow::~MainWindow()
{
    // CAN 清理
    if(can_modu_name == TL_MCANFD) {
        LibUcan2Loader::Deinit();
        LibUcan2Loader::unload();
    }
    else if(can_modu_name == CX_USBCAN) {
        if (cxCan) {
            cxCan->closeDevice(cxDevType, cxDevIndex);
            delete cxCan;
            cxCan = nullptr;
        }
    }
    else if(can_modu_name == ZY_USBCAN) {
        if (zyCan) {
            zyCan->closeDevice(4, 0);
            delete zyCan;
            zyCan = nullptr;
        }
    }

    delete ui;
}

void Delay_MSec(unsigned int msec)
{
    QEventLoop loop;
    QTimer::singleShot(msec,&loop,SLOT(quit()));
    loop.exec();
}

u32 Delay_or_ReadSucc_ms(u32 ms)
{
    read_wait.flag = 0;
    read_wait.num = 0;
    while (1) {
        Delay_MSec(1);
        read_wait.num++;
        if(read_wait.num >= ms)
        {
            return FAIL;       //接收超时
        }
        else if(read_wait.flag > 0)
        {
            return read_wait.flag;
        }
    }
}

//void ReadSucc(void)
//{
//    read_wait.flag=1;
//}


void MainWindow::Read_Data()
{
    QByteArray buf = serial->readAll();
    if (buf.isEmpty()) return;

    m_recvBuf.append(buf);

    bool matched = false;
    switch (m_commStep) {
    case STEP_READY:
        if (m_recvBuf.contains("ready")) {
            read_wait.flag = 1;
            matched = true;
        }
        break;
    case STEP_OK1:
        if (m_recvBuf.contains("OK1")) {
            read_wait.flag = 2;
            matched = true;
        }
        break;
    case STEP_OK3:
        if (m_recvBuf.contains("OK3")) {
            read_wait.flag = 3;
            matched = true;
        }
        break;
    default:
        break;
    }

    if (matched) {
        m_recvBuf.clear();
    }

    if (m_recvBuf.size() > 512) {
        m_recvBuf.clear();
    }
}

u32 MainWindow::comm_test_begin()
{
    if (!serial || !serial->isOpen()) return FAIL;

    m_commStep = STEP_READY;
    m_recvBuf.clear();

    read_wait.flag = 0;
    read_wait.num  = 0;
    u32 res = Delay_or_ReadSucc_ms(COM_RES_TIME);
    if (res != 1) return FAIL;
    serial->write("YES\r\n");

    m_commStep = STEP_OK1;
    read_wait.flag = 0;
    read_wait.num  = 0;
    res = Delay_or_ReadSucc_ms(COM_RES_TIME);
    if (res != 2) return FAIL;
    serial->write("OK2\r\n");

    m_commStep = STEP_OK3;
    read_wait.flag = 0;
    read_wait.num  = 0;
    res = Delay_or_ReadSucc_ms(COM_RES_TIME);
    if (res != 3) return FAIL;

    return SUCC;
}

void MainWindow::SerialPort_init(int port_num)
{
    qDebug() << "[INIT] 进入函数, port_num=" << port_num;

    // 关键：彻底清理旧串口
    if (serial) {
        qDebug() << "[INIT] 清理旧串口...";

        // 先断开所有信号，防止旧对象触发
        disconnect(serial, nullptr, this, nullptr);
        qDebug() << "[INIT] 信号已断开";

        if (serial->isOpen()) {
            qDebug() << "[INIT] 关闭串口...";
            serial->clear();
            serial->close();
        }

        qDebug() << "[INIT] 删除旧对象...";
        delete serial;
        serial = nullptr;

        // 延时让系统释放资源（使用 QElapsedTimer 替代 QThread::msleep）
        qDebug() << "[INIT] 延时等待资源释放...";
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 50) {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        qDebug() << "[INIT] 延时结束，清理完成";
    }

    // 获取串口名
    QString portName;
    QLineEdit* comLine = findChild<QLineEdit*>(QString("COM_Line_%1").arg(port_num));
    if (comLine) portName = comLine->text().trimmed();
    else {
        qDebug() << "[INIT] 未找到 COM_Line_" << port_num;
        return;
    }

    // 检查串口名
    if (portName.isEmpty()) {
        qDebug() << "[INIT] 串口名称为空, port=" << port_num;
        return;
    }

    qDebug() << "[INIT] 准备打开串口:" << portName;

    // 创建新串口对象（指定父对象）
    serial = new QSerialPort(this);

    // 设置串口名（Linux不需要toUpper）
#ifdef Q_OS_WIN
    serial->setPortName(portName.toUpper());
#else
    serial->setPortName(portName);
#endif

    // 打开串口
    if (!serial->open(QIODevice::ReadWrite)) {
        qDebug() << "[INIT] 打开失败:" << portName << ":" << serial->errorString();
        delete serial;
        serial = nullptr;
        return;
    }

    qDebug() << "[INIT] 串口打开成功:" << portName;

    // 设置参数
    serial->setBaudRate(115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    // 清空缓冲区
    serial->clear();

    // 重置状态机
    m_recvBuf.clear();
    m_commStep = STEP_READY;

    // 连接信号（只连接一次）
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_Data);

    qDebug() << "[INIT] 函数结束";
}



void MainWindow::SerialPort_clear()
{
    if (serial) {
        if (serial->isOpen()) {
            disconnect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_Data);
            serial->clear();
            serial->close();
        }
        delete serial;
        serial = nullptr;
    }
    m_recvBuf.clear();
    m_commStep = STEP_READY;
}



void MainWindow::clear_all_line(void)
{
    for (int i = 1; i <= 16; ++i) {
        QLineEdit* comLine = findChild<QLineEdit*>(QString("COM_Line_%1").arg(i));
        QLineEdit* resLine = findChild<QLineEdit*>(QString("COM_TEST_RES_Line_%1").arg(i));
        if (comLine) comLine->clear();
        if (resLine) { resLine->clear(); resLine->setStyleSheet("background-color: white;"); }
    }

    // 清空动态生成的 CAN 显示框
    for (auto &row : m_canRows) {
        if (row.display) {
            row.display->clear();
            row.display->setStyleSheet("background-color: white;");
        }
    }

    Port_num = 0;
}

void MainWindow::clear_com_line(void)
{
    for (int i = 1; i <= 16; ++i) {
        QLineEdit* comLine = findChild<QLineEdit*>(QString("COM_Line_%1").arg(i));
        QLineEdit* resLine = findChild<QLineEdit*>(QString("COM_TEST_RES_Line_%1").arg(i));
        if (comLine) comLine->clear();
        if (resLine) { resLine->clear(); resLine->setStyleSheet("background-color: white;"); }
    }
    ui->com_name->clear();
    QComboBox* cb2 = findChild<QComboBox*>("com_name_2");
    if (cb2) cb2->clear();

    // 清空动态生成的 CAN 显示框
    for (auto &row : m_canRows) {
        if (row.display) {
            row.display->clear();
            row.display->setStyleSheet("background-color: white;");
        }
    }

    Port_num = 0;
}



void MainWindow::scanPortsTo()
{
    for (int i = 1; i <= 16; ++i) {
        QLineEdit* line = findChild<QLineEdit*>(QString("COM_Line_%1").arg(i));
        if (line) line->clear();
    }
    ui->com_name->clear();
    QComboBox* cb2 = findChild<QComboBox*>("com_name_2");
    if (cb2) cb2->clear();

    u32 cnt = 0;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QSerialPort tmp;
        tmp.setPort(info);
        if (tmp.open(QIODevice::ReadWrite)) {
            cnt++; if (cnt > 16) { tmp.close(); break; }
#ifdef Q_OS_WIN
            QString portName = info.portName();
#else
            QString portName = info.systemLocation();
#endif
            QLineEdit* line = findChild<QLineEdit*>(QString("COM_Line_%1").arg(cnt));
            if (line) line->setText(portName);
            ui->com_name->addItem(portName);
            if (cb2) cb2->addItem(portName);
            tmp.close();
        }
    }
    Port_num = cnt;
}

void MainWindow::on_scan_com_btn_clicked() { scanPortsTo(); }

void MainWindow::on_tl_begin_test_btn_clicked()
{
    com_btn_setEnabled(0);

    // 清除所有结果显示
    for (u32 i = 1; i <= Port_num; ++i) {
        QLineEdit* res = findChild<QLineEdit*>(QString("COM_TEST_RES_Line_%1").arg(i));
        if (res) { res->clear(); res->setStyleSheet(""); }
    }

    // 逐个串口测试
    for (u32 i = 1; i <= Port_num; ++i) {
        QLineEdit* comLine = findChild<QLineEdit*>(QString("COM_Line_%1").arg(i));
        if (!comLine || comLine->text().trimmed().isEmpty()) continue;

        SerialPort_init(i);
        QLineEdit* res = findChild<QLineEdit*>(QString("COM_TEST_RES_Line_%1").arg(i));
        if (comm_test_begin() == SUCC) {
            if (res) { res->setText("OK"); res->setStyleSheet("background-color: green;"); }
        } else {
            if (res) { res->setText("ERR"); res->setStyleSheet("background-color: red;"); }
        }
        SerialPort_clear();
    }

    com_btn_setEnabled(1);
}

void MainWindow::com_btn_setEnabled(int con)
{
    bool enable = (con == 1);
    QPushButton* btnScan = findChild<QPushButton*>("scan_com_btn");
    if (btnScan) btnScan->setEnabled(enable);
    QPushButton* btnAll = findChild<QPushButton*>("tl_begin_test_btn");
    if (btnAll) btnAll->setEnabled(enable);
    for (int i = 1; i <= MAX_COM_NUM; ++i) {
        QPushButton* btn = findChild<QPushButton*>(QString("com_test_%1").arg(i));
        if (btn) btn->setEnabled(enable);
    }
    // 动态 CAN 按钮
    for (auto &row : m_canRows) {
        if (row.btnTest) row.btnTest->setEnabled(enable);
    }
}


void MainWindow::testSinglePort(int portNum)
{
    com_btn_setEnabled(0);
    QLineEdit* resLine = findChild<QLineEdit*>(QString("COM_TEST_RES_Line_%1").arg(portNum));
    if (resLine) { resLine->clear(); resLine->setStyleSheet(""); }

    SerialPort_init(portNum);
    if (comm_test_begin() == SUCC) {
        if (resLine) { resLine->setText("OK"); resLine->setStyleSheet("background-color: green;"); }
    } else {
        if (resLine) { resLine->setText("ERR"); resLine->setStyleSheet("background-color: red;"); }
    }
    SerialPort_clear();
    com_btn_setEnabled(1);
}

void MainWindow::on_com_test_1_clicked()  { testSinglePort(1); }
void MainWindow::on_com_test_2_clicked()  { testSinglePort(2); }
void MainWindow::on_com_test_3_clicked()  { testSinglePort(3); }
void MainWindow::on_com_test_4_clicked()  { testSinglePort(4); }
void MainWindow::on_com_test_5_clicked()  { testSinglePort(5); }
void MainWindow::on_com_test_6_clicked()  { testSinglePort(6); }
void MainWindow::on_com_test_7_clicked()  { testSinglePort(7); }
void MainWindow::on_com_test_8_clicked()  { testSinglePort(8); }
void MainWindow::on_com_test_9_clicked()  { testSinglePort(9); }
void MainWindow::on_com_test_10_clicked() { testSinglePort(10); }
void MainWindow::on_com_test_11_clicked() { testSinglePort(11); }
void MainWindow::on_com_test_12_clicked() { testSinglePort(12); }
void MainWindow::on_com_test_13_clicked() { testSinglePort(13); }
void MainWindow::on_com_test_14_clicked() { testSinglePort(14); }
void MainWindow::on_com_test_15_clicked() { testSinglePort(15); }
void MainWindow::on_com_test_16_clicked() { testSinglePort(16); }



// ==================== SocketCAN 动态测试实现 ====================

// 扫描 /sys/class/net 获取所有 can* 接口
QStringList MainWindow::getAvailableCanInterfaces()
{
    QStringList canList;
#ifdef Q_OS_LINUX
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        QStringList filters;
        filters << "can*" << "vcan*";
        canList = netDir.entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot);
        canList.sort();
    }
#endif
    // Windows: 没有 SocketCAN，返回空列表，触发硬件 CAN 模块检测
    return canList;
}

// 初始化动态 CAN 测试界面
void MainWindow::initDynamicCanTests()
{
    QGroupBox *canGroup = ui->groupBox_3;
    if (!canGroup) return;

    // 清除旧布局
    QLayout *oldLayout = canGroup->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete oldLayout;
    }

    // 确定 CAN 接口列表
    QStringList ifaceList;
    if (can_modu_name == NONE) {
        // SocketCAN 模式
        ifaceList = getAvailableCanInterfaces();
    } else {
        // 硬件 CAN 模式：创建 CAN1/CAN2
        ifaceList << "CAN1" << "CAN2";
    }

    QHBoxLayout *mainLayout = new QHBoxLayout(canGroup);
    mainLayout->setContentsMargins(15, 20, 15, 10);
    mainLayout->setSpacing(30);

    if (ifaceList.isEmpty()) {
        QLabel *noCanLabel = new QLabel("⚠️ 未检测到可用的 CAN 接口");
        noCanLabel->setStyleSheet("color: #E53935; font-weight: bold; font-size: 14px;");
        mainLayout->addWidget(noCanLabel, 0, Qt::AlignLeft);
        return;
    }

    QString defaultStatusStyle =
        "QLineEdit {"
        "  background-color: #F5F5F5; color: #757575;"
        "  border: 1px solid #E0E0E0; border-radius: 4px;"
        "  padding: 4px;"
        "}";

    m_canRows.clear();

    for (int i = 0; i < ifaceList.size(); ++i) {
        QString iface = ifaceList.at(i);

        QPushButton *btnTest = new QPushButton(QString("%1 测试").arg(iface));
        btnTest->setFixedSize(100, 30);
        btnTest->setCursor(Qt::PointingHandCursor);

        QLineEdit *statusDisplay = new QLineEdit();
        statusDisplay->setReadOnly(true);
        statusDisplay->setFixedSize(160, 30);
        statusDisplay->setStyleSheet(defaultStatusStyle);
        statusDisplay->setAlignment(Qt::AlignCenter);
        statusDisplay->setText("等待测试...");

        QHBoxLayout *blockLayout = new QHBoxLayout();
        blockLayout->setSpacing(8);
        blockLayout->addWidget(btnTest);
        blockLayout->addWidget(statusDisplay);
        mainLayout->addLayout(blockLayout);

        CanTestRow row = {iface, btnTest, statusDisplay};
        m_canRows.append(row);

        connect(btnTest, &QPushButton::clicked, this, &MainWindow::onCanTestButtonClicked);
    }

    mainLayout->addStretch();
}

// 自动配置 CAN 接口波特率并启用（仅 SocketCAN）
void MainWindow::autoConfigCanInterface(const QString &ifaceName, int bitrate)
{
#ifdef Q_OS_LINUX
    QString downCmd = QString("ip link set down %1").arg(ifaceName);
    QString cfgCmd  = QString("ip link set %1 type can bitrate %2").arg(ifaceName).arg(bitrate);
    QString upCmd   = QString("ip link set up %1").arg(ifaceName);

    system(downCmd.toStdString().c_str());
    system(cfgCmd.toStdString().c_str());
    system(upCmd.toStdString().c_str());

    qDebug() << "Auto configured CAN interface:" << ifaceName << "at bitrate:" << bitrate;
#else
    Q_UNUSED(ifaceName);
    Q_UNUSED(bitrate);
#endif
}

// 执行 CAN 握手测试
void MainWindow::performCanHandshake(const QString &ifaceName, QLineEdit *display)
{
    if (!display) return;

    constexpr uint32_t HANDSHAKE_ID_REQUEST = 0x100;
    constexpr uint32_t HANDSHAKE_ID_REPLY   = 0x101;
    constexpr int HANDSHAKE_ROUNDS = 10;
    constexpr int ROUND_TIMEOUT_MS = 200;
    constexpr int TOTAL_TIMEOUT_MS = 2000;
    constexpr int DATA_LENGTH = 8;

    display->setStyleSheet("background-color: yellow; color: black;");
    display->setText(QString("正在配置 %1 ...").arg(ifaceName));
    QCoreApplication::processEvents();

    // ===== SocketCAN 模式（系统 can0/can1 接口）=====
    if (can_modu_name == NONE) {
        autoConfigCanInterface(ifaceName, 500000);

        display->setText(QString("正在打开接口 %1 ...").arg(ifaceName));
        QCoreApplication::processEvents();

#ifdef Q_OS_LINUX
        int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sock < 0) {
            display->setStyleSheet("background-color: #ffcccc; color: red;");
            display->setText("❌ 创建 Socket 失败");
            return;
        }

        struct ifreq ifr;
        strcpy(ifr.ifr_name, ifaceName.toStdString().c_str());
        if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
            display->setStyleSheet("background-color: #ffcccc; color: red;");
            display->setText("❌ 获取接口索引失败");
            ::close(sock);
            return;
        }

        struct sockaddr_can addr;
        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            display->setStyleSheet("background-color: #ffcccc; color: red;");
            display->setText("❌ 绑定 Socket 失败");
            ::close(sock);
            return;
        }

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        struct can_frame txFrame;
        memset(&txFrame, 0, sizeof(txFrame));
        txFrame.can_id = HANDSHAKE_ID_REQUEST;
        txFrame.can_dlc = DATA_LENGTH;

        QElapsedTimer totalTimer;
        totalTimer.start();

        for (int round = 1; round <= HANDSHAKE_ROUNDS; ++round)
        {
            txFrame.data[0] = round;
            for(int i = 1; i < DATA_LENGTH; ++i) {
                txFrame.data[i] = 0x10 + i;
            }

            if (write(sock, &txFrame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
                display->setStyleSheet("background-color: #ffcccc; color: red;");
                display->setText(QString("❌ 第%1轮发送失败").arg(round));
                ::close(sock);
                return;
            }

            bool replyReceived = false;
            QElapsedTimer roundTimer;
            roundTimer.start();

            display->setText(QString("正在进行第 %1 轮握手...").arg(round));

            while (roundTimer.elapsed() < ROUND_TIMEOUT_MS)
            {
                if (totalTimer.elapsed() >= TOTAL_TIMEOUT_MS) {
                    display->setStyleSheet("background-color: #ffcccc; color: red;");
                    display->setText("⏱️ 总超时2秒，测试中止！");
                    ::close(sock);
                    return;
                }

                struct can_frame rxFrame;
                int nbytes = read(sock, &rxFrame, sizeof(struct can_frame));

                if (nbytes == sizeof(struct can_frame)) {
                    if (rxFrame.can_id == HANDSHAKE_ID_REPLY &&
                        rxFrame.can_dlc == DATA_LENGTH &&
                        rxFrame.data[0] == round)
                    {
                        replyReceived = true;
                        break;
                    }
                }
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }

            if (!replyReceived) {
                display->setStyleSheet("background-color: #ffcccc; color: red;");
                display->setText(QString("❌ 第%1轮超时无回复").arg(round));
                ::close(sock);
                return;
            }
        }

        ::close(sock);
        display->setStyleSheet("background-color: #ccffcc; color: green; font-weight: bold;");
        display->setText("🎉 握手成功！10轮全部完成");
#else
        // Windows 模拟测试
        QElapsedTimer dummyTimer;
        dummyTimer.start();
        for (int round = 1; round <= HANDSHAKE_ROUNDS; ++round) {
            display->setText(QString("Windows 模拟测试: 第 %1 轮...").arg(round));
            while(dummyTimer.elapsed() < 100) { QCoreApplication::processEvents(QEventLoop::AllEvents, 5); }
            dummyTimer.restart();
        }
        display->setStyleSheet("background-color: #ccffcc; color: green; font-weight: bold;");
        display->setText("🎉 (模拟)握手成功！10轮全部完成");
#endif
        return;
    }

    // ===== 硬件 CAN 模式（libucan2 / 创芯 / 致远）=====
    int channel = (ifaceName == "CAN2") ? 2 : 1;

    libucan2_CANFrame txFrame = {0};
    txFrame.MsgId = HANDSHAKE_ID_REQUEST;
    txFrame.IsClassicFrame = true;
    txFrame.UseBRS = false;
    txFrame.IsDataFrame = true;
    txFrame.IsStdId = true;
    txFrame.DataLength = DATA_LENGTH;

    libucan2_CANFrame rxFrame = {0};

    auto sendFrame = [&](libucan2_CANFrame* frame) -> bool {
        switch(can_modu_name) {
        case TL_MCANFD:
            return LibUcan2Loader::SendFrame(channel, frame);
        case CX_USBCAN: {
            if (!cxCan) return false;
            QByteArray data((char*)frame->Data, frame->DataLength);
            return cxCan->sendFrame(cxDevType, cxDevIndex, channel - 1,
                                    frame->MsgId, data,
                                    !frame->IsStdId, !frame->IsDataFrame);
        }
        case ZY_USBCAN: {
            if (!zyCan) return false;
            QByteArray data((char*)frame->Data, frame->DataLength);
            return zyCan->sendFrame(4, 0, channel - 1, frame->MsgId, data,
                                    !frame->IsStdId, !frame->IsDataFrame);
        }
        }
        return false;
    };

    auto recvFrame = [&](libucan2_CANFrame* frame) -> bool {
        switch(can_modu_name) {
        case TL_MCANFD:
            return LibUcan2Loader::RecvFrame(channel, frame);
        case CX_USBCAN: {
            if (!cxCan) return false;
            ChuangXin::VCI_CAN_OBJ vco;
            int ret = cxCan->receive(cxDevType, cxDevIndex, channel - 1, &vco, 1, 0);
            if (ret > 0) {
                frame->MsgId = vco.ID;
                frame->IsStdId = (vco.ExternFlag == 0);
                frame->IsDataFrame = (vco.RemoteFlag == 0);
                frame->DataLength = vco.DataLen;
                memcpy(frame->Data, vco.Data, vco.DataLen);
                return true;
            }
            return false;
        }
        case ZY_USBCAN: {
            if (!zyCan) return false;
            ZhiYuan::VCI_CAN_OBJ vco;
            int ret = zyCan->receive(4, 0, channel - 1, &vco, 1, 0);
            if (ret > 0) {
                frame->MsgId = vco.ID;
                frame->IsStdId = (vco.ExternFlag == 0);
                frame->IsDataFrame = (vco.RemoteFlag == 0);
                frame->DataLength = vco.DataLen;
                memcpy(frame->Data, vco.Data, vco.DataLen);
                return true;
            }
            return false;
        }
        }
        return false;
    };

    QElapsedTimer totalTimer;
    totalTimer.start();

    for(int round = 1; round <= HANDSHAKE_ROUNDS; ++round)
    {
        txFrame.Data[0] = round;
        for(int i = 1; i < DATA_LENGTH; ++i) {
            txFrame.Data[i] = 0x10 + i;
        }

        if(!sendFrame(&txFrame)) {
            display->setText(QString("❌ 第%1轮发送失败").arg(round));
            return;
        }

        bool replyReceived = false;
        QElapsedTimer roundTimer;
        roundTimer.start();

        while(roundTimer.elapsed() < ROUND_TIMEOUT_MS)
        {
            if(totalTimer.elapsed() >= TOTAL_TIMEOUT_MS) {
                display->setText("⏱️ 总超时2秒！");
                return;
            }

            if(recvFrame(&rxFrame)) {
                if(rxFrame.MsgId == HANDSHAKE_ID_REPLY &&
                        rxFrame.DataLength == DATA_LENGTH &&
                        rxFrame.Data[0] == round) {
                    replyReceived = true;
                    break;
                }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        }

        if(!replyReceived) {
            display->setText(QString("❌ 第%1轮超时无回复").arg(round));
            return;
        }
    }

    display->setStyleSheet("background-color: green;");
    display->setText("🎉 握手成功！10轮全部完成");
}

// 动态 CAN 按钮统一槽函数
void MainWindow::onCanTestButtonClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;

    clickedBtn->setEnabled(false);

    QString targetIface;
    QLineEdit *targetDisplay = nullptr;
    for (const CanTestRow &row : m_canRows) {
        if (row.btnTest == clickedBtn) {
            targetIface = row.ifaceName;
            targetDisplay = row.display;
            break;
        }
    }

    if (targetDisplay) {
        performCanHandshake(targetIface, targetDisplay);
    }

    clickedBtn->setEnabled(true);
}
