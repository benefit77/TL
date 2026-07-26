#include "canresponder.h"
#include <QCoreApplication>
#include <QElapsedTimer>

CanResponder::CanResponder(QObject *parent)
    : QObject(parent)
{
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &CanResponder::onPollTimer);
}

CanResponder::~CanResponder()
{
    closeInterface();
}

bool CanResponder::openInterface(const QString &ifaceName)
{
    closeInterface();

    m_ifaceName = ifaceName;
    m_roundCount = 0;

#ifdef Q_OS_LINUX
    // Linux 上优先尝试 SocketCAN
    if (initSocketCan(ifaceName)) {
        m_hwMode = false;
        emit logMessage(QString("✅ CAN 接口 %1 已打开 (SocketCAN)").arg(ifaceName));
        emit statusChanged(QString("🟢 CAN %1 监听中...").arg(ifaceName));

        // 启动轮询定时器（5ms 间隔）
        m_pollTimer->start(5);
        return true;
    }
#endif

    // 没有 SocketCAN 时使用模拟模式（用于 Windows 开发测试）
    m_hwMode = true;
    emit logMessage(QString("⚠️ CAN %1 - 模拟模式（无硬件 CAN 接口）").arg(ifaceName));
    emit statusChanged(QString("🟡 CAN %1 模拟模式").arg(ifaceName));

    // 模拟模式下也用定时器轮询（但每 2 秒提示一次）
    m_pollTimer->start(2000);
    return true;
}

bool CanResponder::initSocketCan(const QString &ifaceName)
{
#ifdef Q_OS_LINUX
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        emit logMessage(QString("创建 CAN 套接字失败: %1").arg(strerror(errno)));
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifaceName.toStdString().c_str(), IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        emit logMessage(QString("获取接口索引 %1 失败: %2").arg(ifaceName).arg(strerror(errno)));
        ::close(sock);
        return false;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        emit logMessage(QString("绑定 CAN 套接字失败: %1").arg(strerror(errno)));
        ::close(sock);
        return false;
    }

    // 设置为非阻塞
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // 设置接收过滤器 - 只接收 ID=0x100 的帧
    struct can_filter filter;
    filter.can_id   = REQUEST_ID;
    filter.can_mask = CAN_SFF_MASK;  // 只匹配标准帧 ID
    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));

    m_fd = sock;
    return true;
#else
    Q_UNUSED(ifaceName);
    return false;
#endif
}

void CanResponder::closeInterface()
{
    m_pollTimer->stop();

#ifdef Q_OS_LINUX
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
#endif

    m_ifaceName.clear();
    m_roundCount = 0;
}

bool CanResponder::isOpen() const
{
    return m_fd >= 0 || m_hwMode;
}

QString CanResponder::interfaceName() const
{
    return m_ifaceName;
}

void CanResponder::onPollTimer()
{
    if (m_hwMode) {
        // 模拟模式：不做实际收，只显示状态
        return;
    }

#ifdef Q_OS_LINUX
    struct can_frame rxFrame;
    int nbytes;

    // 非阻塞读取所有可用帧
    while ((nbytes = read(m_fd, &rxFrame, sizeof(struct can_frame))) == sizeof(struct can_frame)) {
        uint32_t canId = rxFrame.can_id & CAN_ERR_MASK;

        // 记录接收到的帧
        QByteArray frameData((const char*)rxFrame.data, rxFrame.can_dlc);
        emit frameReceived(canId, frameData);
        emit logMessage(QString("← CAN 收到: ID=0x%1 DLC=%2 Data=%3")
                            .arg(canId, 3, 16, QChar('0'))
                            .arg(rxFrame.can_dlc)
                            .arg(frameData.toHex(' ').toUpper()));

        // 自动响应
        if (m_autoRespond && canId == REQUEST_ID) {
            processIncomingFrame(rxFrame.data, rxFrame.can_dlc, canId);
        }
    }

    // nbytes == 0 或 <0 且非 EAGAIN 表示出错
    if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        emit logMessage(QString("⚠️ CAN 读取错误: %1").arg(strerror(errno)));
    }
#endif
}

void CanResponder::processIncomingFrame(const uint8_t *data, int len, uint32_t id)
{
    Q_UNUSED(id);

    // 发送回复帧: ID=0x101, 复制数据载荷
    if (sendReply(REPLY_ID, data, len)) {
        m_roundCount++;

        QByteArray replyData((const char*)data, len);
        emit frameSent(REPLY_ID, replyData);
        emit statusChanged(QString("🟢 已回复第 %1 轮握手").arg(m_roundCount));

        if (m_roundCount >= 10) {
            emit handshakeCompleted(m_roundCount);
            emit statusChanged(QString("🎉 CAN 10 轮握手全部完成！"));
        }
    }
}

bool CanResponder::sendReply(uint32_t id, const uint8_t *data, int len)
{
#ifdef Q_OS_LINUX
    if (m_fd < 0) return false;

    struct can_frame txFrame;
    memset(&txFrame, 0, sizeof(txFrame));
    txFrame.can_id = id;
    txFrame.can_dlc = (len > 8) ? 8 : len;
    memcpy(txFrame.data, data, txFrame.can_dlc);

    if (write(m_fd, &txFrame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        emit logMessage(QString("❌ CAN 发送失败: %1").arg(strerror(errno)));
        return false;
    }

    QByteArray replyData((const char*)txFrame.data, txFrame.can_dlc);
    emit logMessage(QString("→ CAN 发送: ID=0x%1 DLC=%2 Data=%3")
                        .arg(id, 3, 16, QChar('0'))
                        .arg(txFrame.can_dlc)
                        .arg(replyData.toHex(' ').toUpper()));
    return true;
#else
    Q_UNUSED(id);
    Q_UNUSED(data);
    Q_UNUSED(len);
    return false;
#endif
}
