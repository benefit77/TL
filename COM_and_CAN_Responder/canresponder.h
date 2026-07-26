#ifndef CANRESPONDER_H
#define CANRESPONDER_H

#include <QObject>
#include <QTimer>
#include <QDebug>

// SocketCAN (Linux only)
#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#endif

/**
 * @brief CAN 响应器 - DUT 模拟
 *
 * 实现与测试端 COM_and_CAN_TEST 配套的 CAN 握手协议：
 *   1. 监听 ID=0x100 的标准数据帧
 *   2. 收到后回复 ID=0x101，复制 data[0]=round，其余数据不变
 *   3. 共 10 轮握手
 */
class CanResponder : public QObject
{
    Q_OBJECT
public:
    explicit CanResponder(QObject *parent = nullptr);
    ~CanResponder();

    bool openInterface(const QString &ifaceName);
    void closeInterface();
    bool isOpen() const;
    QString interfaceName() const;

    // 设置自动响应模式
    void setAutoRespond(bool enabled) { m_autoRespond = enabled; }
    bool autoRespond() const { return m_autoRespond; }

signals:
    void statusChanged(const QString &status);
    void logMessage(const QString &msg);
    void frameReceived(unsigned int id, const QByteArray &data);
    void frameSent(unsigned int id, const QByteArray &data);
    void handshakeCompleted(int totalRounds);

private slots:
    void onPollTimer();

private:
    int m_fd = -1;              // SocketCAN 套接字
    QString m_ifaceName;
    QTimer *m_pollTimer = nullptr;
    bool m_autoRespond = true;
    int m_roundCount = 0;

    // 硬件 CAN 回退模式（Windows 或无 SocketCAN 时模拟）
    bool m_hwMode = false;

    // 握手协议常量
    static constexpr uint32_t REQUEST_ID  = 0x100;
    static constexpr uint32_t REPLY_ID    = 0x101;
    static constexpr int DATA_LENGTH      = 8;

    bool initSocketCan(const QString &ifaceName);
    void processIncomingFrame(const uint8_t *data, int len, uint32_t id);
    bool sendReply(uint32_t id, const uint8_t *data, int len);
};

#endif // CANRESPONDER_H
