#include "rawudpsocket.h"

// ========== 平台相关头文件 ==========
#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    // Windows 需要初始化 Winsock
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <poll.h>
    #include <sys/ioctl.h>
#endif
#include <string.h>
#include <QDebug>

// Windows 辅助宏
#ifdef Q_OS_WIN
    static bool winsockInitialized = false;
    static bool initWinsock() {
        if (!winsockInitialized) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                return false;
            }
            winsockInitialized = true;
        }
        return true;
    }
    #define SOCKET_ERRNO WSAGetLastError()
    #define SOCKET_ERR_AGAIN WSAEWOULDBLOCK
#else
    #define SOCKET_ERRNO errno
    #define SOCKET_ERR_AGAIN EAGAIN
#endif

RawUdpSocket::RawUdpSocket(QObject *parent)
    : QObject(parent)
    , m_fd(-1)
    , m_notifier(nullptr)
{
#ifdef Q_OS_WIN
    initWinsock();
#endif
}

RawUdpSocket::~RawUdpSocket()
{
    close();
}

bool RawUdpSocket::bind(const QHostAddress &address, quint16 port, const QString &deviceName)
{
    close();

    m_deviceName = deviceName;

#ifdef Q_OS_WIN
    // Windows: 使用普通 UDP socket，不能绑定到特定网口设备名
    SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == INVALID_SOCKET) {
        setError(QString("socket() failed: %1").arg(WSAGetLastError()));
        return false;
    }
    m_fd = static_cast<int>(s);
#else
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0) {
        setError(QString("socket() failed: %1").arg(strerror(errno)));
        return false;
    }

    // Linux: 绑定到指定网口
    if (!deviceName.isEmpty()) {
        QByteArray devName = deviceName.toUtf8();
        if (setsockopt(m_fd, SOL_SOCKET, SO_BINDTODEVICE,
                       devName.constData(), devName.length()) < 0) {
            setError(QString("SO_BINDTODEVICE(%1) failed: %2").arg(deviceName).arg(strerror(errno)));
            close();
            return false;
        }
    }
#endif

    // 设置地址重用
    int reuse = 1;
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    // 绑定到指定IP
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address.toIPv4Address());

    if (::bind(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef Q_OS_WIN
        setError(QString("bind(%1:%2) failed: %3").arg(address.toString()).arg(port).arg(WSAGetLastError()));
#else
        setError(QString("bind(%1:%2) failed: %3").arg(address.toString()).arg(port).arg(strerror(errno)));
#endif
        close();
        return false;
    }

    // 设置非阻塞
#ifdef Q_OS_WIN
    u_long nonblock = 1;
    if (ioctlsocket(m_fd, FIONBIO, &nonblock) != 0) {
        setError(QString("ioctlsocket failed: %1").arg(WSAGetLastError()));
        close();
        return false;
    }
#else
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        setError(QString("fcntl failed: %1").arg(strerror(errno)));
        close();
        return false;
    }
#endif

    // 创建socket notifier
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &RawUdpSocket::onSocketActivated);
    m_notifier->setEnabled(true);

    return true;
}

void RawUdpSocket::close()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_fd >= 0) {
#ifdef Q_OS_WIN
        ::closesocket(m_fd);
#else
        ::close(m_fd);
#endif
        m_fd = -1;
    }
    m_deviceName.clear();
    m_error.clear();
}

qint64 RawUdpSocket::writeDatagram(const QByteArray &data, const QHostAddress &host, quint16 port)
{
    if (m_fd < 0) {
        setError("Socket not open");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(host.toIPv4Address());

#ifdef Q_OS_WIN
    int sent = sendto(m_fd, data.constData(), data.size(), 0,
                      (struct sockaddr*)&addr, sizeof(addr));
#else
    ssize_t sent = sendto(m_fd, data.constData(), data.size(), 0,
                          (struct sockaddr*)&addr, sizeof(addr));
#endif

    if (sent < 0) {
#ifdef Q_OS_WIN
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        setError(QString("sendto failed: %1").arg(err));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        setError(QString("sendto failed: %1").arg(strerror(errno)));
#endif
        return -1;
    }
    return sent;
}

bool RawUdpSocket::hasPendingDatagrams() const
{
    if (m_fd < 0) return false;

#ifdef Q_OS_WIN
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    struct timeval tv = {0, 0};
    return select(m_fd + 1, &fds, nullptr, nullptr, &tv) > 0;
#else
    struct pollfd pfd;
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    return poll(&pfd, 1, 0) > 0;
#endif
}

qint64 RawUdpSocket::pendingDatagramSize() const
{
    if (m_fd < 0) return -1;

    int size;
#ifdef Q_OS_WIN
    if (ioctlsocket(m_fd, FIONREAD, (u_long*)&size) < 0) return -1;
#else
    if (ioctl(m_fd, FIONREAD, &size) < 0) return -1;
#endif
    return size;
}

qint64 RawUdpSocket::readDatagram(char *data, qint64 maxSize, QHostAddress *host, quint16 *port)
{
    if (m_fd < 0) {
        setError("Socket not open");
        return -1;
    }

    struct sockaddr_in addr;
#ifdef Q_OS_WIN
    int addrLen = sizeof(addr);
#else
    socklen_t addrLen = sizeof(addr);
#endif

#ifdef Q_OS_WIN
    int recv = recvfrom(m_fd, data, maxSize, 0,
                        (struct sockaddr*)&addr, &addrLen);
#else
    ssize_t recv = recvfrom(m_fd, data, maxSize, 0,
                            (struct sockaddr*)&addr, &addrLen);
#endif

    if (recv < 0) {
#ifdef Q_OS_WIN
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        setError(QString("recvfrom failed: %1").arg(err));
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        setError(QString("recvfrom failed: %1").arg(strerror(errno)));
#endif
        return -1;
    }

    if (host) *host = QHostAddress(ntohl(addr.sin_addr.s_addr));
    if (port) *port = ntohs(addr.sin_port);

    return recv;
}

void RawUdpSocket::onSocketActivated()
{
    emit readyRead();
}

void RawUdpSocket::setError(const QString &err)
{
    m_error = err;
    qDebug() << "RawUdpSocket[" << m_deviceName << "] Error:" << err;
}
