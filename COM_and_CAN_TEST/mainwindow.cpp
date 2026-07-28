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

// Windows 注册表识别物理网卡
#ifdef Q_OS_WIN
#include <QSettings>
#endif

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

    // 初始化网络测试（动态检测网卡）
    initDynamicNetTests();

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
        filters << "can*";
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

// ==================== UDP 网络测试实现（动态网卡扫描）====================

// Windows: 判断是否物理以太网卡（排除蓝牙、虚拟网卡）
static bool isPhysicalEthernet(const QString &ifaceName)
{
#ifdef Q_OS_WIN
    QNetworkInterface ni = QNetworkInterface::interfaceFromName(ifaceName);
    if (!ni.isValid()) return false;

    // ====== 1. 名称关键字快速过滤（含英文和中文关键词） ======
    QString nameAll = (ni.humanReadableName() + " " + ifaceName + " " +
                       ni.hardwareAddress()).toLower();
    QStringList quickExclude = {
        "bluetooth", "virtualbox", "vmware",
        "vgate", "hostonly", "pseudo",
        "personal area network",  // 蓝牙 PAN 专有
        "bluetooth设备", "蓝牙",   // 中文蓝牙
    };
    for (const QString &pat : quickExclude) {
        if (nameAll.contains(pat)) return false;
    }

    // ====== 2. MAC 地址过滤 ======
    QString mac = ni.hardwareAddress().toLower().replace(":", "").replace("-", "");
    if (mac.isEmpty() || mac == "0000000000000") return false;

    // 常见虚拟/蓝牙 MAC 前缀
    QStringList virtualMacPrefixes = {
        "080027", "0a0027",           // VirtualBox
        "000569", "0050c2", "001c42", // VMware
        "001e10",                     // Bluetooth 某些厂商
    };
    for (const QString &prefix : virtualMacPrefixes) {
        if (mac.startsWith(prefix)) return false;
    }

    // ====== 3. 读注册表 DriverDesc（真实硬件描述） ======
    // 路径: HKLM\...\Class\{4d36e972-...}\<索引>\DriverDesc
    QSettings reg("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\"
                  "{4d36e972-e325-11ce-bfc1-08002be10318}", QSettings::NativeFormat);

    // 同时检查 Network 连接注册表路径
    QString netCfgPath = "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Network\\"
                         "{4d36e972-e325-11ce-bfc1-08002be10318}";
    QSettings regNet(netCfgPath, QSettings::NativeFormat);

    QStringList descs;
    QStringList groups = reg.childGroups();
    for (const QString &key : groups) {
        reg.beginGroup(key);
        QString desc = reg.value("DriverDesc").toString().toLower();
        QString guid = reg.value("NetCfgInstanceId").toString().toLower();
        reg.endGroup();

        if (guid.isEmpty()) continue;

        // 去掉大括号再比较
        QString guidClean = guid.trimmed().remove('{').remove('}');
        QString ifaceClean = ifaceName.trimmed().remove('{').remove('}');
        if (ifaceClean.contains(guidClean, Qt::CaseInsensitive)) {
            descs << desc;
            // 检查描述
            QStringList excludeDesc = {
                "bluetooth", "virtualbox", "vmware",
                "host-only", "hostonly", "pseudo",
                "npcap", "npcai",
                "microsoft wi-fi direct", "microsoft kernel debug",
                "tunnel", "ras sync", "wan miniport",
                "ndisip", "packet scheduler",
                "personal area network",
            };
            for (const QString &pat : excludeDesc) {
                if (desc.contains(pat)) return false;
            }
            return true; // 注册表确认是物理网卡
        }
    }

    // ====== 4. 注册表没匹配到 ======
    // 再用 humanReadableName + name + MAC 做最终过滤
    QString fullName = (ni.humanReadableName() + " " + ifaceName + " " +
                        ni.hardwareAddress()).toLower();
    QStringList fallbackExclude = {"bluetooth", "virtualbox", "vmware",
                                   "vgate", "pseudo", "loopback",
                                   "personal area network", "bluetooth设备", "蓝牙"};
    for (const QString &pat : fallbackExclude) {
        if (fullName.contains(pat)) return false;
    }

    return true;
#else
    Q_UNUSED(ifaceName);
    return true;
#endif
}

QStringList MainWindow::getAvailableNetInterfaces()
{
    QStringList ifaceList;
#ifdef Q_OS_LINUX
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        // 只保留有线以太网接口，排除 wlan/wifi/蓝牙
        QStringList filters;
        filters << "eth*" << "enp*" << "enx*" << "eno*";
        ifaceList = netDir.entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot);
        ifaceList.sort();
    }
#endif
    // Windows: 用 QNetworkInterface + Windows API 枚举
    if (ifaceList.isEmpty()) {
        foreach (const QNetworkInterface &ni, QNetworkInterface::allInterfaces()) {
            // 排除回环
            if (ni.flags().testFlag(QNetworkInterface::IsLoopBack))
                continue;
            if (ni.type() == QNetworkInterface::Loopback ||
                ni.type() == QNetworkInterface::Wifi)
                continue;

#ifdef Q_OS_WIN
            // 用 Windows API 确认是物理以太网卡
            if (!isPhysicalEthernet(ni.name()))
                continue;
#endif

            ifaceList << ni.name();
        }
    }
    return ifaceList;
}

// 获取网卡的第一个 IPv4 地址
static QString getInterfaceIp(const QString &ifaceName)
{
    QNetworkInterface ni = QNetworkInterface::interfaceFromName(ifaceName);
    if (!ni.isValid()) return QString();
    foreach (const QNetworkAddressEntry &entry, ni.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
            !entry.ip().isLoopback()) {
            return entry.ip().toString();
        }
    }
    return QString();
}

// 初始化动态网络测试界面（纯代码生成，更大更清晰）
void MainWindow::initDynamicNetTests()
{
    QGroupBox *netGroup = ui->groupBox_5;
    if (!netGroup) return;

    // 清除旧布局
    QLayout *oldLayout = netGroup->layout();
    if (oldLayout) {
        QLayoutItem *item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete oldLayout;
    }

    m_netRows.clear();

    // ====== 整体垂直布局 ======
    QVBoxLayout *mainLayout = new QVBoxLayout(netGroup);
    mainLayout->setContentsMargins(10, 8, 10, 8);
    mainLayout->setSpacing(6);

    // ====== 配置行：端口 | 扫描网卡 ======
    QHBoxLayout *cfgRow = new QHBoxLayout();
    cfgRow->setSpacing(8);

    QLabel *lblPort = new QLabel("端口:");
    lblPort->setStyleSheet("font-size: 13px; font-weight: bold;");

    QLineEdit *edtPort = new QLineEdit();
    edtPort->setObjectName("netTargetPort");
    edtPort->setText("12346");
    edtPort->setFixedSize(70, 28);
    edtPort->setStyleSheet("font-size: 13px; padding: 2px 6px;");

    QPushButton *btnScan = new QPushButton("🔍 扫描网卡");
    btnScan->setFixedSize(100, 30);
    btnScan->setCursor(Qt::PointingHandCursor);
    btnScan->setStyleSheet(
        "QPushButton { font-size: 12px; font-weight: bold;"
        "  background-color: #E3F2FD; border: 1px solid #90CAF9;"
        "  border-radius: 4px; padding: 4px 8px; }"
        "QPushButton:hover { background-color: #BBDEFB; }");

    cfgRow->addWidget(lblPort);
    cfgRow->addWidget(edtPort);
    cfgRow->addStretch();
    cfgRow->addWidget(btnScan);
    mainLayout->addLayout(cfgRow);

    // ====== 分隔线 ======
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #E0E0E0;");
    mainLayout->addWidget(line);

    // ====== 网卡列表 ======
    QStringList ifaceList = getAvailableNetInterfaces();

    if (ifaceList.isEmpty()) {
        QLabel *noIface = new QLabel("⚠️ 未检测到可用的网卡");
        noIface->setStyleSheet("color: #E53935; font-weight: bold; font-size: 14px; padding: 10px;");
        mainLayout->addWidget(noIface);
        connect(btnScan, &QPushButton::clicked, this, &MainWindow::onNetScanClicked);
        return;
    }

    // 列宽常量：与数据行严格对应
    const int COL_TEST    = 90;   // 连通测试按钮
    const int COL_BW      = 50;   // 带宽测试按钮
    const int COL_IP      = 150;  // IP 地址
    const int COL_TARGET  = 120;  // 目标IP
    const int COL_RESULT  = 130;  // 连通结果
    const int COL_BW_RES  = 140;  // 带宽结果

    // ====== 表头（使用 QGridLayout 确保与数据行列对齐）======
    // 先用一个水平布局装表头，每列宽度与数据行完全一致
    QHBoxLayout *headerRow = new QHBoxLayout();
    headerRow->setSpacing(8);

    QLabel *hdrIface = new QLabel("网口");
    hdrIface->setFixedWidth(COL_TEST);
    hdrIface->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    QLabel *hdrBwBtn = new QLabel("带宽");
    hdrBwBtn->setFixedWidth(COL_BW);
    hdrBwBtn->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    QLabel *hdrIp = new QLabel("IP 地址");
    hdrIp->setFixedWidth(COL_IP);
    hdrIp->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    QLabel *hdrTarget = new QLabel("目标IP");
    hdrTarget->setFixedWidth(COL_TARGET);
    hdrTarget->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    QLabel *hdrResult = new QLabel("连通测试");
    hdrResult->setFixedWidth(COL_RESULT);
    hdrResult->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    QLabel *hdrBw = new QLabel("带宽测试");
    hdrBw->setFixedWidth(COL_BW_RES);
    hdrBw->setStyleSheet("font-weight: bold; font-size: 12px; color: #757575;");

    headerRow->addWidget(hdrIface);
    headerRow->addWidget(hdrBwBtn);
    headerRow->addWidget(hdrIp);
    headerRow->addWidget(hdrTarget);
    headerRow->addWidget(hdrResult);
    headerRow->addWidget(hdrBw);
    headerRow->addStretch();
    mainLayout->addLayout(headerRow);

    // 网卡行
    QString defaultStyle =
        "QLineEdit { background-color: #FAFAFA; color: #616161;"
        "  border: 1px solid #E0E0E0; border-radius: 4px;"
        "  font-size: 13px; padding: 4px 8px; }";

    for (int i = 0; i < ifaceList.size(); ++i) {
        QString iface = ifaceList.at(i);
        QString ip = getInterfaceIp(iface);
        if (ip.isEmpty()) ip = "无IP";

        // 获取 Windows 风格名称："以太网", "以太网 2", "以太网 3" ...
        QNetworkInterface ni = QNetworkInterface::interfaceFromName(iface);
        QString friendlyName = ni.humanReadableName();
        if (friendlyName.isEmpty()) friendlyName = iface;

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(8);

        // 连通测试按钮
        QPushButton *btnTest = new QPushButton(QString("%1").arg(friendlyName));
        btnTest->setFixedSize(COL_TEST, 32);
        btnTest->setCursor(Qt::PointingHandCursor);
        btnTest->setToolTip(QString("系统接口: %1").arg(iface));
        btnTest->setStyleSheet(
            "QPushButton { font-size: 11px; font-weight: bold;"
            "  background-color: #E8F5E9; border: 1px solid #A5D6A7;"
            "  border-radius: 4px; padding: 0 4px; }"
            "QPushButton:hover { background-color: #C8E6C9; }"
            "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }");

        // 带宽测试按钮
        QPushButton *btnBwTest = new QPushButton("带宽");
        btnBwTest->setFixedSize(COL_BW, 32);
        btnBwTest->setCursor(Qt::PointingHandCursor);
        btnBwTest->setToolTip("发送大流量 UDP 包测试网卡带宽");
        btnBwTest->setStyleSheet(
            "QPushButton { font-size: 11px; font-weight: bold;"
            "  background-color: #E3F2FD; border: 1px solid #90CAF9;"
            "  border-radius: 4px; padding: 0 4px; }"
            "QPushButton:hover { background-color: #BBDEFB; }"
            "QPushButton:disabled { background-color: #F5F5F5; color: #BDBDBD; }");

        // IP 地址（用只读 QLineEdit 代替 QLabel，避免文字截断）
        QLineEdit *lblIpAddr = new QLineEdit(ip);
        lblIpAddr->setReadOnly(true);
        lblIpAddr->setFixedWidth(COL_IP);
        lblIpAddr->setStyleSheet(
            "QLineEdit { color: #1565C0; font-weight: bold; font-size: 13px;"
            "  background: transparent; border: none; padding: 4px; }");

        // 每行独立的目标IP输入框
        QLineEdit *targetIpInput = new QLineEdit();
        targetIpInput->setFixedSize(COL_TARGET, 28);
        targetIpInput->setStyleSheet(
            "QLineEdit { background-color: #FFF8E1; color: #333;"
            "  border: 1px solid #FFE082; border-radius: 4px;"
            "  font-size: 13px; padding: 2px 6px; }");
        {
            QString suggested;
            QStringList parts = ip.split('.');
            if (parts.size() == 4 && ip != "无IP") {
                suggested = parts[0] + "." + parts[1] + "." + parts[2] + ".100";
            }
            targetIpInput->setText(suggested);
            targetIpInput->setPlaceholderText("目标IP");
        }

        // 连通测试结果框
        QLineEdit *display = new QLineEdit();
        display->setReadOnly(true);
        display->setFixedSize(COL_RESULT, 30);
        display->setStyleSheet(defaultStyle);
        display->setAlignment(Qt::AlignCenter);
        display->setText("-");

        // 带宽测试结果框
        QLineEdit *bwDisplay = new QLineEdit();
        bwDisplay->setReadOnly(true);
        bwDisplay->setFixedSize(COL_BW_RES, 30);
        bwDisplay->setStyleSheet(defaultStyle);
        bwDisplay->setAlignment(Qt::AlignCenter);
        bwDisplay->setText("-");

        // 隐藏的延迟显示
        QLineEdit *latencyDisp = new QLineEdit();
        latencyDisp->setFixedWidth(0);
        latencyDisp->hide();

        row->addWidget(btnTest);
        row->addWidget(btnBwTest);
        row->addWidget(lblIpAddr);
        row->addWidget(targetIpInput);
        row->addWidget(display);
        row->addWidget(bwDisplay);
        row->addStretch();
        mainLayout->addLayout(row);

        NetTestRow netRow = {iface, ip, btnTest, btnBwTest, display, latencyDisp, bwDisplay, targetIpInput};
        m_netRows.append(netRow);

        connect(btnTest, &QPushButton::clicked, this, &MainWindow::onNetTestButtonClicked);
        connect(btnBwTest, &QPushButton::clicked, this, &MainWindow::onNetBwTestButtonClicked);
    }

    mainLayout->addStretch();

    // 连接扫描按钮
    connect(btnScan, &QPushButton::clicked, this, &MainWindow::onNetScanClicked);
}

void MainWindow::onNetScanClicked()
{
    // 重新扫描并刷新界面
    initDynamicNetTests();
}

void MainWindow::performNetHandshake(const QString &ifaceName, const QString &srcIp,
                                      const QString &targetIp, quint16 targetPort,
                                      QLineEdit *display, QLineEdit *latencyDisplay)
{
    if (!display) return;

    constexpr int HANDSHAKE_ROUNDS = 10;
    constexpr int ROUND_TIMEOUT_MS = 500;
    constexpr int TOTAL_TIMEOUT_MS = 10000;
    constexpr quint16 LOCAL_PORT = 22345;

    display->setStyleSheet("background-color: yellow; color: black;");
    display->setText("连接中...");
    QCoreApplication::processEvents();

    // 创建 UDP socket，绑定到指定网卡
    RawUdpSocket udpSocket;
    if (!udpSocket.bind(QHostAddress::AnyIPv4, LOCAL_PORT, ifaceName)) {
        display->setStyleSheet("background-color: #ffcccc; color: red;");
        display->setText("❌ 绑定失败");
        return;
    }

    QHostAddress remoteAddr(targetIp);

    int successCount = 0;
    double totalLatency = 0;
    QElapsedTimer totalTimer;
    totalTimer.start();

    for (int round = 1; round <= HANDSHAKE_ROUNDS; ++round)
    {
        QByteArray reqData;
        reqData.append("NET_PING");
        reqData.append((char*)&round, sizeof(int));

        qint64 sendTime = QDateTime::currentMSecsSinceEpoch();
        reqData.append((char*)&sendTime, sizeof(qint64));

        if (udpSocket.writeDatagram(reqData, remoteAddr, targetPort) < 0) {
            display->setStyleSheet("background-color: #ffcccc; color: red;");
            display->setText(QString("❌ 第%1轮发送失败").arg(round));
            return;
        }

        bool replyReceived = false;
        QElapsedTimer roundTimer;
        roundTimer.start();

        while (roundTimer.elapsed() < ROUND_TIMEOUT_MS)
        {
            if (totalTimer.elapsed() >= TOTAL_TIMEOUT_MS) {
                display->setStyleSheet("background-color: #ffcccc; color: red;");
                display->setText("⏱️ 超时！");
                return;
            }

            if (udpSocket.hasPendingDatagrams())
            {
                char buf[64];
                QHostAddress peerAddr;
                quint16 peerPort;
                qint64 n = udpSocket.readDatagram(buf, sizeof(buf), &peerAddr, &peerPort);

                if (n > 0)
                {
                    QByteArray rspData(buf, n);
                    if (rspData.startsWith("NET_PONG") && rspData.size() >= 12)
                    {
                        int rspRound;
                        memcpy(&rspRound, rspData.constData() + 8, sizeof(int));
                        if (rspRound == round)
                        {
                            replyReceived = true;
                            qint64 now = QDateTime::currentMSecsSinceEpoch();
                            double latency = (now - sendTime) / 1000.0;
                            totalLatency += latency;
                            successCount++;

                            display->setText(QString("%1/%2 ✓ %3ms")
                                                 .arg(round).arg(HANDSHAKE_ROUNDS)
                                                 .arg(latency * 1000, 0, 'f', 1));
                            break;
                        }
                    }
                }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        }

        if (!replyReceived) {
            display->setStyleSheet("background-color: #ffcccc; color: red;");
            display->setText(QString("❌ 第%1轮超时").arg(round));
            return;
        }
    }

    double avgLatency = (successCount > 0) ? (totalLatency / successCount) : 0;
    display->setStyleSheet("background-color: #ccffcc; color: green; font-weight: bold;");
    display->setText(QString("🎉 %1/%2 ✓ 平均%3ms")
                         .arg(successCount).arg(HANDSHAKE_ROUNDS)
                         .arg(avgLatency * 1000, 0, 'f', 1));
}

// ============================================
// 带宽测试：向目标发送大量 UDP 包，测量发送吞吐量
// ============================================
void MainWindow::performNetBwTest(const QString &ifaceName, const QString &targetIp,
                                   quint16 targetPort, QLineEdit *bwDisplay)
{
    if (!bwDisplay) return;

    // 测试参数
    constexpr int PACKET_SIZE = 1400;       // UDP 载荷大小（避免 IP 分片）
    constexpr int TEST_DURATION_MS = 3000;  // 发包持续时间 (3秒)
    constexpr quint16 LOCAL_PORT = 22347;   // 本地端口（与连通测试不同）
    constexpr int REPORT_TIMEOUT_MS = 2000; // 等待 BWRPT 报告超时

    bwDisplay->setStyleSheet("background-color: #FFF3E0; color: #E65100; font-weight: bold;");
    bwDisplay->setText("⏳ 测速中...");
    QCoreApplication::processEvents();

    // 创建并绑定 socket
    RawUdpSocket udpSocket;
    if (!udpSocket.bind(QHostAddress::AnyIPv4, LOCAL_PORT, ifaceName)) {
        bwDisplay->setStyleSheet("background-color: #ffcccc; color: red;");
        bwDisplay->setText("❌ 绑定失败");
        return;
    }

    QHostAddress remoteAddr(targetIp);

    // 预分配数据包（避免循环内反复构造）
    QByteArray packet(PACKET_SIZE, 0);
    packet.fill('D');
    packet.replace(0, 6, "BWTEST");

    // ====== Phase 1: 发送突发流量，持续 TEST_DURATION_MS 毫秒 ======
    qint64 totalBytes = 0;
    int totalPackets = 0;
    int seq = 0;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < TEST_DURATION_MS) {
        memcpy(packet.data() + 6, &seq, sizeof(int));
        qint64 ts = QDateTime::currentMSecsSinceEpoch();
        memcpy(packet.data() + 10, &ts, sizeof(qint64));

        qint64 sent = udpSocket.writeDatagram(packet, remoteAddr, targetPort);
        if (sent > 0) {
            totalBytes += sent;
            totalPackets++;
            seq++;
        } else if (sent == 0) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        } else {
            break;
        }

        if (totalPackets % 100 == 0) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
    }

    qint64 burstEndMs = timer.elapsed();

    // ====== Phase 2: 发送 BWEND 命令，通知目标端统计结束 ======
    QByteArray endCmd;
    endCmd.append("BWEND");
    endCmd.append((char*)&totalPackets, 4);
    endCmd.append((char*)&totalBytes, 8);
    udpSocket.writeDatagram(endCmd, remoteAddr, targetPort);

    // ====== Phase 3: 等待目标端回复 BWRPT 报告 ======
    int recvPackets = 0;
    qint64 recvBytes = 0;
    int lostPackets = 0;
    qint64 durMs = 0;
    bool gotReport = false;

    QElapsedTimer waitTimer;
    waitTimer.start();
    while (waitTimer.elapsed() < REPORT_TIMEOUT_MS) {
        if (udpSocket.hasPendingDatagrams()) {
            char buf[64];
            QHostAddress peerAddr;
            quint16 peerPort;
            qint64 n = udpSocket.readDatagram(buf, sizeof(buf), &peerAddr, &peerPort);
            if (n > 0) {
                QByteArray rsp(buf, n);
                if (rsp.startsWith("BWRPT") && n >= 25) {
                    memcpy(&recvPackets, rsp.constData() + 5, 4);
                    memcpy(&recvBytes,  rsp.constData() + 9, 8);
                    memcpy(&lostPackets, rsp.constData() + 17, 4);
                    memcpy(&durMs,       rsp.constData() + 21, 8);
                    gotReport = true;
                    break;
                }
            }
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }

    udpSocket.close();

    // ====== Phase 4: 计算并显示结果 ======
    double mbps = 0;
    if (gotReport && durMs > 0) {
        // 使用目标端报告的接收字节数和时长
        mbps = (recvBytes * 8.0) / (durMs / 1000.0) / 1000000.0;
    } else if (burstEndMs > 0) {
        // 没收到报告，回退到纯发送速率
        mbps = (totalBytes * 8.0) / (burstEndMs / 1000.0) / 1000000.0;
    }

    int lossRate = (totalPackets > 0) ? (lostPackets * 100 / totalPackets) : 0;

    QString result;
    if (gotReport) {
        if (mbps >= 1000.0)
            result = QString("⇄ %1 Gbps").arg(mbps / 1000.0, 0, 'f', 2);
        else
            result = QString("⇄ %1 Mbps").arg(mbps, 0, 'f', 1);

        result += QString(" 收%1/%2包").arg(recvPackets).arg(totalPackets);
        if (lostPackets > 0)
            result += QString(" 丢%1(%2%)").arg(lostPackets).arg(lossRate);
        result += QString(" %3s").arg(durMs / 1000.0, 0, 'f', 1);
    } else {
        // 目标端未回复 BWRPT → 显示发送速率（单向）
        if (mbps >= 1000.0)
            result = QString("↑%1 Gbps").arg(mbps / 1000.0, 0, 'f', 2);
        else
            result = QString("↑%1 Mbps").arg(mbps, 0, 'f', 1);
        result += QString(" (%1包) ⚠️ 无报告").arg(totalPackets);
    }

    bwDisplay->setStyleSheet(
        "QLineEdit { background-color: #E8F5E9; color: #2E7D32;"
        "  font-weight: bold; border: 1px solid #A5D6A7;"
        "  border-radius: 4px; font-size: 12px; padding: 4px 8px; }");
    bwDisplay->setText(result);

    qDebug() << "带宽测试完成:" << ifaceName
             << "发送" << totalPackets << "包" << totalBytes << "字节"
             << (gotReport ? QString("接收%1包 丢%2 %3Mbps").arg(recvPackets).arg(lostPackets).arg(mbps, 0, 'f', 1)
                           : "未收到目标报告");
}

void MainWindow::onNetBwTestButtonClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;

    clickedBtn->setEnabled(false);

    // 端口仍是全局共用
    QLineEdit *portEdit = findChild<QLineEdit*>("netTargetPort");
    if (!portEdit) { clickedBtn->setEnabled(true); return; }

    quint16 targetPort = portEdit->text().toUShort();
    if (targetPort == 0) {
        clickedBtn->setEnabled(true);
        return;
    }

    // 找到对应的网卡行，使用该行自己的目标IP
    for (const NetTestRow &row : m_netRows) {
        if (row.btnBwTest == clickedBtn) {
            QString targetIp;
            if (row.targetIpInput) {
                targetIp = row.targetIpInput->text().trimmed();
            }
            if (targetIp.isEmpty()) {
                row.bwDisplay->setStyleSheet("background-color: #ffcccc; color: red;");
                row.bwDisplay->setText("❌ 请填写目标IP");
                clickedBtn->setEnabled(true);
                return;
            }
            performNetBwTest(row.ifaceName, targetIp, targetPort, row.bwDisplay);
            break;
        }
    }

    clickedBtn->setEnabled(true);
}

void MainWindow::onNetTestButtonClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (!clickedBtn) return;

    clickedBtn->setEnabled(false);

    // 端口仍是全局共用
    QLineEdit *portEdit = findChild<QLineEdit*>("netTargetPort");
    if (!portEdit) { clickedBtn->setEnabled(true); return; }

    quint16 targetPort = portEdit->text().toUShort();
    if (targetPort == 0) {
        clickedBtn->setEnabled(true);
        return;
    }

    // 找到对应的网卡行，使用该行自己的目标IP
    for (const NetTestRow &row : m_netRows) {
        if (row.btnTest == clickedBtn) {
            QString targetIp;
            if (row.targetIpInput) {
                targetIp = row.targetIpInput->text().trimmed();
            }
            if (targetIp.isEmpty()) {
                row.display->setStyleSheet("background-color: #ffcccc; color: red;");
                row.display->setText("❌ 请填写目标IP");
                clickedBtn->setEnabled(true);
                return;
            }
            performNetHandshake(row.ifaceName, row.ipAddress,
                                targetIp, targetPort,
                                row.display, row.latencyDisplay);
            break;
        }
    }

    clickedBtn->setEnabled(true);
}
