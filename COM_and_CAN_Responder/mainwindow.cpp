#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("COM & CAN Responder (DUT 模拟器)");

    // 创建响应器对象
    m_serialResp = new SerialResponder(this);
    m_canResp = new CanResponder(this);
    m_netResp = new NetResponder(this);

    // 连接串口响应器信号
    connect(m_serialResp, &SerialResponder::statusChanged,
            this, &MainWindow::onSerialStatus);
    connect(m_serialResp, &SerialResponder::logMessage,
            this, &MainWindow::onSerialLog);
    connect(m_serialResp, &SerialResponder::handshakeCompleted,
            this, &MainWindow::onSerialHandshakeDone);

    // 连接 CAN 响应器信号
    connect(m_canResp, &CanResponder::statusChanged,
            this, &MainWindow::onCanStatus);
    connect(m_canResp, &CanResponder::logMessage,
            this, &MainWindow::onCanLog);
    connect(m_canResp, &CanResponder::handshakeCompleted,
            this, &MainWindow::onCanHandshakeDone);

    // 连接网络响应器信号
    connect(m_netResp, &NetResponder::statusChanged,
            this, &MainWindow::onNetStatus);
    connect(m_netResp, &NetResponder::logMessage,
            this, &MainWindow::onNetLog);
    connect(m_netResp, &NetResponder::handshakeCompleted,
            this, &MainWindow::onNetHandshakeDone);

    // 初始化端口列表
    initSerialPorts();
    initCanInterfaces();

    // 初始状态
    ui->btnCloseCom->setEnabled(false);
    ui->btnCloseCan->setEnabled(false);
    ui->btnStopNet->setEnabled(false);
    ui->comStatusLabel->setText("🔴 未连接");
    ui->canStatusLabel->setText("🔴 未连接");
    ui->netStatusLabel->setText("🔴 未启动");
    ui->netPortInput->setText("12346");

    appendLog("🚀 COM & CAN & NET Responder 启动");
    appendLog("等待选择串口或 CAN 接口后开始响应...");
}

MainWindow::~MainWindow()
{
    if (m_serialResp) m_serialResp->closePort();
    if (m_canResp) m_canResp->closeInterface();
    if (m_netResp) m_netResp->stopListen();
    delete ui;
}

// ==================== 串口部分 ====================

void MainWindow::initSerialPorts()
{
    ui->comSelector->clear();
    ui->comSelector->addItem("— 选择串口 —", "");

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString desc = info.description().trimmed();
        QString label = info.portName();
        if (!desc.isEmpty())
            label += QString(" (%1)").arg(desc);
        ui->comSelector->addItem(label, info.portName());
    }

    ui->comSelector->setCurrentIndex(0);
}

void MainWindow::on_btnScanCom_clicked()
{
    QString currentPort = ui->comSelector->currentData().toString();
    initSerialPorts();

    // 恢复之前选中的端口
    for (int i = 0; i < ui->comSelector->count(); ++i) {
        if (ui->comSelector->itemData(i).toString() == currentPort) {
            ui->comSelector->setCurrentIndex(i);
            break;
        }
    }

    appendLog("🔄 已刷新串口列表");
}

void MainWindow::on_btnOpenCom_clicked()
{
    QString portName = ui->comSelector->currentData().toString();
    if (portName.isEmpty()) {
        appendLog("⚠️ 请先选择一个串口");
        return;
    }

    if (m_serialResp->openPort(portName, 115200)) {
        ui->btnOpenCom->setEnabled(false);
        ui->btnCloseCom->setEnabled(true);
        ui->comSelector->setEnabled(false);
        ui->btnScanCom->setEnabled(false);
    }
}

void MainWindow::on_btnCloseCom_clicked()
{
    m_serialResp->closePort();
    ui->btnOpenCom->setEnabled(true);
    ui->btnCloseCom->setEnabled(false);
    ui->comSelector->setEnabled(true);
    ui->btnScanCom->setEnabled(true);
    ui->comStatusLabel->setText("🔴 未连接");
    appendLog("🔌 串口已关闭");
}

void MainWindow::on_comSelector_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // 可在此处更新串口详情
}

// ==================== CAN 部分 ====================

QStringList MainWindow::getAvailableCanInterfaces()
{
    QStringList canList;
#ifdef Q_OS_LINUX
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        QStringList filters;
        filters << "can*" << "vcan*" << "slcan*";
        canList = netDir.entryList(filters, QDir::Dirs | QDir::NoDotAndDotDot);
        canList.sort();
    }
#endif
    return canList;
}

void MainWindow::initCanInterfaces()
{
    ui->canInterfaceSelector->clear();
    ui->canInterfaceSelector->addItem("— 选择 CAN 接口 —", "");

    // 添加 SocketCAN 接口
    QStringList canIfaces = getAvailableCanInterfaces();
    for (const QString &iface : canIfaces) {
        ui->canInterfaceSelector->addItem(iface, iface);
    }

    // 添加虚拟 CAN 选项（Windows 开发测试用）
    ui->canInterfaceSelector->addItem("CAN1 (模拟)", "CAN1");
    ui->canInterfaceSelector->addItem("CAN2 (模拟)", "CAN2");

    ui->canInterfaceSelector->setCurrentIndex(0);
}

void MainWindow::on_btnScanCan_clicked()
{
    QString currentIface = ui->canInterfaceSelector->currentData().toString();
    initCanInterfaces();

    // 恢复之前选中的接口
    for (int i = 0; i < ui->canInterfaceSelector->count(); ++i) {
        if (ui->canInterfaceSelector->itemData(i).toString() == currentIface) {
            ui->canInterfaceSelector->setCurrentIndex(i);
            break;
        }
    }

    appendLog("🔄 已刷新 CAN 接口列表");
}

void MainWindow::on_btnOpenCan_clicked()
{
    QString ifaceName = ui->canInterfaceSelector->currentData().toString();
    if (ifaceName.isEmpty()) {
        appendLog("⚠️ 请先选择一个 CAN 接口");
        return;
    }

    if (m_canResp->openInterface(ifaceName)) {
        ui->btnOpenCan->setEnabled(false);
        ui->btnCloseCan->setEnabled(true);
        ui->canInterfaceSelector->setEnabled(false);
        ui->btnScanCan->setEnabled(false);
    }
}

void MainWindow::on_btnCloseCan_clicked()
{
    m_canResp->closeInterface();
    ui->btnOpenCan->setEnabled(true);
    ui->btnCloseCan->setEnabled(false);
    ui->canInterfaceSelector->setEnabled(true);
    ui->btnScanCan->setEnabled(true);
    ui->canStatusLabel->setText("🔴 未连接");
    appendLog("🔌 CAN 接口已关闭");
}

void MainWindow::on_canInterfaceSelector_currentIndexChanged(int index)
{
    Q_UNUSED(index);
}

// ==================== 网络部分 ====================

void MainWindow::on_btnStartNet_clicked()
{
    quint16 port = ui->netPortInput->text().toUShort();
    if (port == 0) {
        appendLog("⚠️ 请输入有效的端口号");
        return;
    }

    if (m_netResp->startListen(port)) {
        ui->btnStartNet->setEnabled(false);
        ui->btnStopNet->setEnabled(true);
        ui->netPortInput->setEnabled(false);
    }
}

void MainWindow::on_btnStopNet_clicked()
{
    m_netResp->stopListen();
    ui->btnStartNet->setEnabled(true);
    ui->btnStopNet->setEnabled(false);
    ui->netPortInput->setEnabled(true);
    ui->netStatusLabel->setText("🔴 未启动");
    appendLog("🔌 网络监听已关闭");
}

// ==================== 自动响应开关 ====================

void MainWindow::on_checkBoxAutoRespond_toggled(bool checked)
{
    m_canResp->setAutoRespond(checked);
    appendLog(checked ? "✅ CAN 自动响应已开启" : "⏸️ CAN 自动响应已关闭");
}

// ==================== 网络信号处理 ====================

void MainWindow::onNetStatus(const QString &status)
{
    ui->netStatusLabel->setText(status);
}

void MainWindow::onNetLog(const QString &msg)
{
    appendLog("[网络] " + msg);
}

void MainWindow::onNetHandshakeDone(int rounds)
{
    appendLog(QString("🎉 网络 %1 轮握手全部完成！").arg(rounds));
}

// ==================== 日志 ====================

void MainWindow::on_btnClearLog_clicked()
{
    ui->logList->clear();
}

void MainWindow::appendLog(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui->logList->addItem(QString("[%1] %2").arg(timestamp).arg(msg));
    ui->logList->scrollToBottom();
}

// ==================== 信号处理 ====================

void MainWindow::onSerialStatus(const QString &status)
{
    ui->comStatusLabel->setText(status);
}

void MainWindow::onSerialLog(const QString &msg)
{
    appendLog("[串口] " + msg);
}

void MainWindow::onSerialHandshakeDone()
{
    appendLog("🎉 串口握手协议全部完成！");
}

void MainWindow::onCanStatus(const QString &status)
{
    ui->canStatusLabel->setText(status);
}

void MainWindow::onCanLog(const QString &msg)
{
    appendLog("[CAN] " + msg);
}

void MainWindow::onCanHandshakeDone(int rounds)
{
    appendLog(QString("🎉 CAN %1 轮握手全部完成！").arg(rounds));
}
