#ifndef NETRESPONDER_H
#define NETRESPONDER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QTimer>
#include <QDebug>
#include <QDateTime>

/**
 * @brief 网络响应器 - DUT 模拟 (UDP Ping-Pong + 带宽测试)
 *
 * 实现与测试端 COM_and_CAN_TEST 配套的 UDP 网络测试协议：
 *   1. 收到 "NET_PING" + round(4字节) + timestamp(8字节)
 *      → 自动回复 "NET_PONG" + round(4字节)
 *   2. 收到 "BWTEST"   + seq(4字节) + timestamp(8字节) + 填充
 *      → 统计接收包数和字节数
 *   3. 收到 "BWEND"   + total_sent(4字节) + total_bytes(8字节)
 *      → 回复 "BWRPT" + recv_pkts(4) + recv_bytes(8) + lost(4) + dur_ms(8)
 */
class NetResponder : public QObject
{
    Q_OBJECT
public:
    explicit NetResponder(QObject *parent = nullptr);
    ~NetResponder();

    bool startListen(quint16 port = 12346);
    void stopListen();
    bool isListening() const;
    quint16 listenPort() const { return m_port; }

signals:
    void statusChanged(const QString &status);
    void logMessage(const QString &msg);
    void pingReceived(int round, const QString &fromAddr, quint16 fromPort);
    void pongSent(int round);
    void handshakeCompleted(int totalRounds);
    void bwTestCompleted(int recvPackets, qint64 recvBytes, int lostPackets, double mbps);

private slots:
    void onReadyRead();

private:
    QUdpSocket *m_socket = nullptr;
    quint16 m_port = 0;
    int m_roundCount = 0;
    bool m_autoRespond = true;

    // 带宽测试状态（每个客户端独立跟踪）
    struct BwSession {
        quint32 clientIp = 0;
        quint16 clientPort = 0;
        int recvPackets = 0;
        qint64 recvBytes = 0;
        qint64 startMs = 0;
        bool active = false;
    } m_bw;

    void sendPong(const QHostAddress &addr, quint16 port, int round);
    void handleBwTestPacket(const QByteArray &datagram, qint64 n,
                            const QHostAddress &senderAddr, quint16 senderPort);
    void handleBwEnd(const QByteArray &datagram,
                     const QHostAddress &senderAddr, quint16 senderPort);
    void sendBwReport(const QHostAddress &addr, quint16 port,
                      int sentPackets, qint64 sentBytes);
};

#endif // NETRESPONDER_H
