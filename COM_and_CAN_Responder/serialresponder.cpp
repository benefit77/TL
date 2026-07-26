#include "serialresponder.h"
#include <QCoreApplication>

SerialResponder::SerialResponder(QObject *parent)
    : QObject(parent)
{
}

SerialResponder::~SerialResponder()
{
    closePort();
}

bool SerialResponder::openPort(const QString &portName, int baudRate)
{
    closePort();

    m_serial = new QSerialPort(this);

#ifdef Q_OS_WIN
    m_serial->setPortName(portName.toUpper());
#else
    m_serial->setPortName(portName);
#endif

    if (!m_serial->open(QIODevice::ReadWrite)) {
        QString err = QString("打开串口 %1 失败: %2").arg(portName).arg(m_serial->errorString());
        emit logMessage(err);
        emit statusChanged("❌ " + err);
        delete m_serial;
        m_serial = nullptr;
        return false;
    }

    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    m_serial->clear();

    connect(m_serial, &QSerialPort::readyRead, this, &SerialResponder::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialResponder::onErrorOccurred);

    m_recvBuf.clear();
    m_readySent = false;

    emit logMessage(QString("✅ 串口 %1 已打开, 波特率 %2").arg(portName).arg(baudRate));
    emit statusChanged(QString("🟢 已连接 %1").arg(portName));

    // 启动后立即发送 "ready"
    sendReady();

    return true;
}

void SerialResponder::closePort()
{
    if (m_serial) {
        if (m_serial->isOpen()) {
            m_serial->clear();
            m_serial->close();
        }
        delete m_serial;
        m_serial = nullptr;
    }
    m_recvBuf.clear();
    m_readySent = false;
}

bool SerialResponder::isOpen() const
{
    return m_serial && m_serial->isOpen();
}

QString SerialResponder::portName() const
{
    if (m_serial && m_serial->isOpen())
        return m_serial->portName();
    return QString();
}

void SerialResponder::sendReady()
{
    if (!m_serial || !m_serial->isOpen()) return;

    QByteArray cmd = "ready\r\n";
    qint64 written = m_serial->write(cmd);
    if (written > 0) {
        m_serial->flush();
        m_readySent = true;
        emit logMessage("→ 发送: ready");
        emit statusChanged("🟡 已发送 ready，等待 YES...");
    }
}

void SerialResponder::sendOk1()
{
    if (!m_serial || !m_serial->isOpen()) return;

    QByteArray cmd = "OK1\r\n";
    m_serial->write(cmd);
    m_serial->flush();
    emit logMessage("→ 发送: OK1");
    emit statusChanged("🟡 已发送 OK1，等待 OK2...");
}

void SerialResponder::sendOk3()
{
    if (!m_serial || !m_serial->isOpen()) return;

    QByteArray cmd = "OK3\r\n";
    m_serial->write(cmd);
    m_serial->flush();
    emit logMessage("→ 发送: OK3");
    emit statusChanged("🟢 握手完成！");
    emit handshakeCompleted();
}

void SerialResponder::onReadyRead()
{
    if (!m_serial) return;

    QByteArray buf = m_serial->readAll();
    m_recvBuf.append(buf);

    // 记录收到的原始数据
    QString hexStr;
    for (int i = 0; i < buf.size(); ++i) {
        hexStr += QString("%1 ").arg((unsigned char)buf[i], 2, 16, QChar('0'));
    }
    QString textStr = QString::fromUtf8(buf).trimmed();
    emit logMessage(QString("← 收到: [%1] \"%2\"").arg(hexStr.trimmed()).arg(textStr));

    // 检查是否收到 "YES"（测试端在收到 ready 后发送 YES）
    if (m_recvBuf.contains("YES")) {
        m_recvBuf.clear();
        emit logMessage("✓ 识别到 YES，准备回复 OK1");
        sendOk1();
        return;
    }

    // 检查是否收到 "OK2"（测试端在收到 OK1 后发送 OK2）
    if (m_recvBuf.contains("OK2")) {
        m_recvBuf.clear();
        emit logMessage("✓ 识别到 OK2，准备回复 OK3");
        sendOk3();
        return;
    }

    // 防止接收缓冲区无限增长
    if (m_recvBuf.size() > 512) {
        m_recvBuf.clear();
    }
}

void SerialResponder::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;

    QString errMsg = QString("串口错误: %1").arg(m_serial ? m_serial->errorString() : "未知");
    emit logMessage("❌ " + errMsg);
    emit statusChanged("🔴 " + errMsg);

    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        // 串口被拔出或断开，自动清理
        closePort();
    }
}
