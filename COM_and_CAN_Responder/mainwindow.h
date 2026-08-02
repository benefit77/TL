#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>
#include <QListWidget>

#include "serialresponder.h"
#include "canresponder.h"
#include "netresponder.h"

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
    // 串口相关
    void on_btnScanCom_clicked();
    void on_btnOpenCom_clicked();
    void on_btnCloseCom_clicked();
    void on_comSelector_currentIndexChanged(int index);

    // CAN 相关
    void on_btnOpenCan_clicked();
    void on_btnCloseCan_clicked();
    void on_btnScanCan_clicked();
    void on_canInterfaceSelector_currentIndexChanged(int index);

    // 网络相关
    void on_btnStartNet_clicked();
    void on_btnStopNet_clicked();

    // 日志
    void on_btnClearLog_clicked();

    // 响应器信号
    void onSerialStatus(const QString &status);
    void onSerialLog(const QString &msg);
    void onSerialHandshakeDone();
    void onSerialPortDisconnected();   // 串口异常断开时复位界面
    void onCanStatus(const QString &status);
    void onCanLog(const QString &msg);
    void onCanHandshakeDone(int rounds);

    // 网络响应器信号
    void onNetStatus(const QString &status);
    void onNetLog(const QString &msg);
    void onNetHandshakeDone(int rounds);

    // 自动回复全部
    void on_checkBoxAutoRespond_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    SerialResponder *m_serialResp = nullptr;
    CanResponder *m_canResp = nullptr;
    NetResponder *m_netResp = nullptr;

    void initSerialPorts();
    void initCanInterfaces();
    void appendLog(const QString &msg);
    QStringList getAvailableCanInterfaces();
};

#endif // MAINWINDOW_H
