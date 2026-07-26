#include "netresponder.h"

NetResponder::NetResponder(QObject *parent)
    : QObject(parent)
{
    memset(&m_bw, 0, sizeof(m_bw));
}

NetResponder::~NetResponder()
{
    stopListen();
}

bool NetResponder::startListen(quint16 port)
{
    stopListen();

    m_socket = new QUdpSocket(this);
    m_port = port;
    m_roundCount = 0;
    memset(&m_bw, 0, sizeof(m_bw));

    if (!m_socket->bind(QHostAddress::AnyIPv4, port)) {
        QString err = QString("绑定 UDP 端口 %1 失败: %2").arg(port).arg(m_socket->errorString());
        emit logMessage("❌ " + err);
        emit statusChanged("🔴 " + err);
        delete m_socket;
        m_socket = nullptr;
        m_port = 0;
        return false;
    }

    connect(m_socket, &QUdpSocket::readyRead, this, &NetResponder::onReadyRead);

    emit logMessage(QString("✅ UDP 端口 %1 已打开，等待网络测试...").arg(port));
    emit statusChanged(QString("🟢 监听端口 %1").arg(port));
    return true;
}

void NetResponder::stopListen()
{
    if (m_socket) {
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
    }
    m_port = 0;
    m_roundCount = 0;
    memset(&m_bw, 0, sizeof(m_bw));
}

bool NetResponder::isListening() const
{
    return m_socket && m_socket->isOpen();
}

void NetResponder::onReadyRead()
{
    if (!m_socket) return;

    while (m_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(m_socket->pendingDatagramSize());
        QHostAddress senderAddr;
        quint16 senderPort;

        qint64 n = m_socket->readDatagram(datagram.data(), datagram.size(),
                                          &senderAddr, &senderPort);
        if (n <= 0) continue;

        // ====== 1. 标准 Ping-Pong 测试 ======
        if (datagram.startsWith("NET_PING") && datagram.size() >= 20)
        {
            int round;
            memcpy(&round, datagram.constData() + 8, sizeof(int));

            qint64 sendTime;
            memcpy(&sendTime, datagram.constData() + 12, sizeof(qint64));
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            double rtt = (now - sendTime) / 1000.0;

            QString fromStr = QString("%1:%2").arg(senderAddr.toString()).arg(senderPort);
            emit pingReceived(round, senderAddr.toString(), senderPort);
            emit logMessage(QString("← NET_PING 第%1轮 来自 %2 (RTT=%.1fms)")
                                .arg(round).arg(fromStr).arg(rtt * 1000));

            sendPong(senderAddr, senderPort, round);
        }
        // ====== 2. 带宽测试数据包 ======
        else if (datagram.startsWith("BWTEST") && n >= 18)
        {
            handleBwTestPacket(datagram, n, senderAddr, senderPort);
        }
        // ====== 3. 带宽测试结束 ======
        else if (datagram.startsWith("BWEND") && n >= 13)
        {
            handleBwEnd(datagram, senderAddr, senderPort);
        }
    }
}

// ============================================
// 带宽测试数据包处理
// ============================================
void NetResponder::handleBwTestPacket(const QByteArray &datagram, qint64 n,
                                       const QHostAddress &senderAddr, quint16 senderPort)
{
    quint32 senderIp = senderAddr.toIPv4Address();

    // 新会话或不同客户端 → 重置
    if (!m_bw.active || m_bw.clientIp != senderIp || m_bw.clientPort != senderPort) {
        m_bw.clientIp = senderIp;
        m_bw.clientPort = senderPort;
        m_bw.recvPackets = 0;
        m_bw.recvBytes = 0;
        m_bw.startMs = QDateTime::currentMSecsSinceEpoch();
        m_bw.active = true;
    }

    m_bw.recvPackets++;
    m_bw.recvBytes += n;
}

// ============================================
// 带宽测试结束 - 发送报告回客户端
// ============================================
void NetResponder::handleBwEnd(const QByteArray &datagram,
                                const QHostAddress &senderAddr, quint16 senderPort)
{
    int sentPackets;
    qint64 sentBytes;
    memcpy(&sentPackets, datagram.constData() + 5, 4);
    memcpy(&sentBytes,  datagram.constData() + 9, 8);

    if (!m_bw.active) {
        // 没有进行中的带宽测试，返回空报告
        sendBwReport(senderAddr, senderPort, sentPackets, sentBytes);
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 durMs = now - m_bw.startMs;
    if (durMs < 1) durMs = 1;

    double mbps = (m_bw.recvBytes * 8.0) / (durMs / 1000.0) / 1000000.0;

    emit logMessage(QString("📊 带宽测试完成: 收%1/%2包 %3MB, 丢%4, %5Mbps")
                        .arg(m_bw.recvPackets).arg(sentPackets)
                        .arg(m_bw.recvBytes / 1048576.0, 0, 'f', 2)
                        .arg(sentPackets - m_bw.recvPackets)
                        .arg(mbps, 0, 'f', 1));

    // 发送报告
    sendBwReport(senderAddr, senderPort, sentPackets, sentBytes);

    emit bwTestCompleted(m_bw.recvPackets, m_bw.recvBytes,
                         sentPackets - m_bw.recvPackets, mbps);
    emit statusChanged(QString("🎉 带宽: %1 Mbps").arg(mbps, 0, 'f', 1));

    // 重置状态
    memset(&m_bw, 0, sizeof(m_bw));
}

// ============================================
// 发送带宽测试报告 "BWRPT"
// ============================================
void NetResponder::sendBwReport(const QHostAddress &addr, quint16 port,
                                 int sentPackets, qint64 sentBytes)
{
    if (!m_socket) return;

    int recvPackets = m_bw.recvPackets;
    qint64 recvBytes = m_bw.recvBytes;
    int lostPackets = sentPackets - recvPackets;
    if (lostPackets < 0) lostPackets = 0;
    qint64 durMs = m_bw.active ? (QDateTime::currentMSecsSinceEpoch() - m_bw.startMs) : 0;

    QByteArray rpt;
    rpt.append("BWRPT");
    rpt.append((char*)&recvPackets, 4);
    rpt.append((char*)&recvBytes, 8);
    rpt.append((char*)&lostPackets, 4);
    rpt.append((char*)&durMs, 8);

    m_socket->writeDatagram(rpt, addr, port);

    emit logMessage(QString("→ BWRPT 报告已发送至 %1:%2")
                        .arg(addr.toString()).arg(port));
}

void NetResponder::sendPong(const QHostAddress &addr, quint16 port, int round)
{
    if (!m_socket) return;

    QByteArray rspData;
    rspData.append("NET_PONG");
    rspData.append((char*)&round, sizeof(int));

    qint64 sent = m_socket->writeDatagram(rspData, addr, port);
    if (sent > 0) {
        m_roundCount++;
        emit pongSent(round);
        emit logMessage(QString("→ NET_PONG 第%1轮 发送至 %2:%3")
                            .arg(round).arg(addr.toString()).arg(port));
        emit statusChanged(QString("🟢 已回复第 %1 轮").arg(m_roundCount));

        if (m_roundCount >= 10) {
            emit handshakeCompleted(m_roundCount);
            emit statusChanged(QString("🎉 网络 10 轮握手全部完成！"));
            m_roundCount = 0;
        }
    } else {
        emit logMessage(QString("❌ NET_PONG 发送失败: %1").arg(m_socket->errorString()));
    }
}
