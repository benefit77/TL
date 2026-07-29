#ifndef RAWUDPSOCKET_H
#define RAWUDPSOCKET_H

#include <QObject>
#include <QHostAddress>
#include <QByteArray>
#include <QSocketNotifier>

class RawUdpSocket : public QObject
{
    Q_OBJECT
public:
    explicit RawUdpSocket(QObject *parent = nullptr);
    ~RawUdpSocket();

    // 绑定到指定IP和网口设备名（如"enp1s0"）
    bool bind(const QHostAddress &address, quint16 port, const QString &deviceName);
    void close();
    bool isValid() const { return m_fd >= 0; }

    qint64 writeDatagram(const QByteArray &data, const QHostAddress &host, quint16 port);
    bool hasPendingDatagrams() const;
    qint64 readDatagram(char *data, qint64 maxSize, QHostAddress *host = nullptr, quint16 *port = nullptr);
    qint64 pendingDatagramSize() const;

    // 在指定毫秒内等待数据可读（替代 processEvents 忙等，精确且低延迟）
    bool waitForReadyRead(int timeoutMs);

    int socketDescriptor() const { return m_fd; }
    QString errorString() const { return m_error; }

signals:
    void readyRead();

private slots:
    void onSocketActivated();

private:
    int m_fd;
    QString m_error;
    QString m_deviceName;
    QSocketNotifier *m_notifier;
    mutable QByteArray m_buffer;  // 用于预读数据长度

    void setError(const QString &err);
};

#endif // RAWUDPSOCKET_H
