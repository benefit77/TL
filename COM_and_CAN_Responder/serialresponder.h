#ifndef SERIALRESPONDER_H
#define SERIALRESPONDER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>

/**
 * @brief 串口响应器 - DUT 模拟
 *
 * 实现与测试端 COM_and_CAN_TEST 配套的握手协议：
 *   1. 连接后主动发送 "ready\r\n"
 *   2. 收到 "YES\r\n" 后回复 "OK1\r\n"
 *   3. 收到 "OK2\r\n" 后回复 "OK3\r\n"
 */
class SerialResponder : public QObject
{
    Q_OBJECT
public:
    explicit SerialResponder(QObject *parent = nullptr);
    ~SerialResponder();

    bool openPort(const QString &portName, int baudRate = 115200);
    void closePort();
    bool isOpen() const;
    QString portName() const;

signals:
    void statusChanged(const QString &status);
    void logMessage(const QString &msg);
    void handshakeCompleted();

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onRetryReady();

private:
    QSerialPort *m_serial = nullptr;
    QByteArray m_recvBuf;
    bool m_readySent = false;

    QTimer *m_retryTimer = nullptr;       ///< 重发 "ready" 的定时器

    static constexpr int RETRY_INTERVAL_MS = 100;     ///< "ready" 重发间隔

    void sendReady();
    void stopTimers();
    void sendOk1();
    void sendOk3();
};

#endif // SERIALRESPONDER_H
