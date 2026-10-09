#include "mainwindow.h"
#include "tca9554.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const char *kHigh    = "#1a7f37";   // 高电平: 绿
const char *kLow     = "#9e9e9e";   // 低电平: 灰
const char *kUnknown = "#c8c8c8";
const char *kOk      = "#1a7f37";
const char *kErr     = "#c62828";

const char *kValueColorFmt = "color:%1;";

QString stateColor(int v)
{
    if (v == 1) return QLatin1String(kHigh);
    if (v == 0) return QLatin1String(kLow);
    return QLatin1String(kUnknown);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("东莞市拓朗工控设备有限公司  GPIO测试程序"));
    buildUi();
    connectDevice();
}

MainWindow::~MainWindow()
{
    delete m_io;
}

void MainWindow::buildUi()
{
    //-------------------------------------------------------------------------
    //  header : ● 已连接 · 地址            (右) 最近一次操作
    //-------------------------------------------------------------------------
    m_conn = new QLabel(this);
    m_status = new QLabel(this);
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_status->setStyleSheet(QStringLiteral("color:#7a7a7a;"));

    QHBoxLayout *header = new QHBoxLayout;
    header->addWidget(m_conn, 0);
    header->addStretch(1);
    header->addWidget(m_status, 0);

    //-------------------------------------------------------------------------
    //  输出 (DO) : 值放在框里, 点击框取反   |   输入 (DI) : 值放在框里, 只读
    //-------------------------------------------------------------------------
    QGroupBox *outGroup = new QGroupBox(QStringLiteral("输出"), this);
    QGridLayout *outGrid = new QGridLayout(outGroup);
    outGrid->setHorizontalSpacing(22);
    outGrid->setVerticalSpacing(24);
    outGrid->setContentsMargins(24, 20, 24, 24);

    QGroupBox *inGroup = new QGroupBox(QStringLiteral("输入"), this);
    QGridLayout *inGrid = new QGridLayout(inGroup);
    inGrid->setHorizontalSpacing(22);
    inGrid->setVerticalSpacing(24);
    inGrid->setContentsMargins(24, 20, 24, 24);

    for (int i = 0; i < Tca9554::kDoCount; ++i) {
        QLabel *name = new QLabel(
            QStringLiteral("OUT%1  (DO%1  bit%2)").arg(i + 1).arg(Tca9554::doBit(i)),
            outGroup);

        QPushButton *box = new QPushButton(QStringLiteral("0"), outGroup);
        box->setObjectName(QStringLiteral("doBox"));
        box->setFixedSize(59, 34);
        box->setFocusPolicy(Qt::NoFocus);
        box->setCursor(Qt::PointingHandCursor);
        box->setToolTip(QStringLiteral("点击取反：0 -> 1，1 -> 0"));
        connect(box, &QPushButton::clicked, this, &MainWindow::onDoClicked);

        m_doBox[i] = box;
        outGrid->addWidget(name, i, 0);
        outGrid->addWidget(box, i, 1, Qt::AlignLeft);
    }
    outGrid->setColumnStretch(2, 1);
    outGrid->setRowStretch(Tca9554::kDoCount, 1);

    for (int i = 0; i < Tca9554::kDiCount; ++i) {
        QLabel *name = new QLabel(
            QStringLiteral("IN%1  (DI%1  bit%2)").arg(i + 1).arg(Tca9554::diBit(i)),
            inGroup);

        QLabel *box = new QLabel(QStringLiteral("--"), inGroup);
        box->setObjectName(QStringLiteral("diBox"));
        box->setAlignment(Qt::AlignCenter);
        box->setFixedSize(59, 34);

        m_diBox[i] = box;
        inGrid->addWidget(name, i, 0);
        inGrid->addWidget(box, i, 1, Qt::AlignLeft);
    }
    inGrid->setColumnStretch(2, 1);
    inGrid->setRowStretch(Tca9554::kDiCount, 1);

    QHBoxLayout *center = new QHBoxLayout;
    center->addWidget(outGroup);
    center->addWidget(inGroup);
    center->addStretch(1);

    //-------------------------------------------------------------------------
    //  bottom : 寄存器原始值      [全部输出清零] [关闭]
    //-------------------------------------------------------------------------
    m_reg = new QLabel(this);
    m_reg->setObjectName(QStringLiteral("regLabel"));

    m_allOff = new QPushButton(QStringLiteral("全部输出清零"), this);
    connect(m_allOff, &QPushButton::clicked, this, &MainWindow::onAllOff);

    QPushButton *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->addWidget(m_reg, 0);
    bottom->addStretch(1);
    bottom->addWidget(m_allOff, 0);
    bottom->addWidget(closeBtn, 0);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(28, 20, 28, 20);
    root->setSpacing(14);
    root->addLayout(header);
    root->addLayout(center, 1);
    root->addLayout(bottom);

    setMinimumSize(900, 520);

    setStyleSheet(QStringLiteral(
        "QDialog { font-size: 16px; }"
        "QGroupBox { font-size: 17px; font-weight: bold; border: 1px solid palette(mid);"
        "            border-radius: 6px; margin-top: 16px; padding: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; }"
        "QLabel { font-size: 16px; }"
        "QLabel#diBox { font-size: 16px; font-weight: bold; background: #ffffff;"
        "               border: 1px solid #9a9a9a; border-radius: 3px; }"
        "QPushButton#doBox { font-size: 16px; font-weight: bold; background: #ffffff;"
        "                    border: 1px solid #9a9a9a; border-radius: 3px; padding: 0; }"
        "QPushButton#doBox:hover { border-color: #1a7f37; }"
        "QPushButton#doBox:disabled { color: #b0b0b0; }"
        "QLabel#regLabel { font-family: monospace; font-size: 15px; color: #7a7a7a; }"
        "QPushButton { font-size: 16px; padding: 7px 16px; }"));
}

void MainWindow::connectDevice()
{
    m_io = new Tca9554(Tca9554::kDefaultAddress);

    QString err;
    if (!m_io->open(&err)) {
        setConnected(false);
        m_status->setText(err);
        m_status->setStyleSheet(QStringLiteral("color:%1;font-weight:bold;").arg(kErr));
        for (int i = 0; i < Tca9554::kDoCount; ++i) {
            m_doBox[i]->setEnabled(false);
            m_doBox[i]->setText(QStringLiteral("?"));
            m_doBox[i]->setStyleSheet(QString::fromLatin1(kValueColorFmt).arg(kUnknown));
        }
        for (int i = 0; i < Tca9554::kDiCount; ++i) {
            m_diBox[i]->setText(QStringLiteral("--"));
            m_diBox[i]->setStyleSheet(QString::fromLatin1(kValueColorFmt).arg(kUnknown));
        }
        m_allOff->setEnabled(false);
        m_reg->setText(QStringLiteral("IN=0x--   OUT=0x--   CFG=0x--"));
        return;
    }

    m_io->configureDirections();

    for (int i = 0; i < Tca9554::kDoCount; ++i) {
        int v = m_io->readDo(i);
        if (v >= 0)
            applyDoState(i, v);
    }

    setConnected(true);
    m_status->clear();

    m_timer = new QTimer(this);
    m_timer->setInterval(500);        // 每 500 ms 刷新
    connect(m_timer, &QTimer::timeout, this, &MainWindow::refreshIo);
    m_timer->start();
    refreshIo();
}

void MainWindow::setConnected(bool ok)
{
    if (ok) {
        m_conn->setText(QStringLiteral("\u25CF 已连接 · SMBus 地址 0x%1")
                            .arg(m_io->address(), 2, 16, QChar('0')));
        m_conn->setStyleSheet(QStringLiteral("color:%1;font-weight:bold;").arg(kOk));
    } else {
        m_conn->setText(QStringLiteral("\u25CF 未连接"));
        m_conn->setStyleSheet(QStringLiteral("color:%1;font-weight:bold;").arg(kErr));
    }
}

void MainWindow::applyDoState(int index, int value)
{
    value = value ? 1 : 0;
    m_doBox[index]->setText(QString::number(value));
    m_doBox[index]->setStyleSheet(QString::fromLatin1(kValueColorFmt).arg(stateColor(value)));
}

void MainWindow::setDiState(int index, int value)
{
    m_diBox[index]->setText(value < 0 ? QStringLiteral("--") : QString::number(value));
    m_diBox[index]->setStyleSheet(QString::fromLatin1(kValueColorFmt).arg(stateColor(value)));
}

void MainWindow::onDoClicked()
{
    QPushButton *box = qobject_cast<QPushButton *>(sender());
    if (!box)
        return;

    int index = -1;
    for (int i = 0; i < Tca9554::kDoCount; ++i)
        if (m_doBox[i] == box)
            index = i;
    if (index < 0)
        return;

    // 点击取反：1 -> 0，0 -> 1
    writeDo(index, box->text().toInt() ? 0 : 1);
}

void MainWindow::onAllOff()
{
    if (!m_io || !m_io->isOpen())
        return;

    for (int i = 0; i < Tca9554::kDoCount; ++i) {
        m_io->writeDo(i, 0);
        applyDoState(i, 0);
    }
    m_status->setText(QStringLiteral("已全部输出清零"));
    refreshIo();
}

void MainWindow::writeDo(int index, int value)
{
    value = value ? 1 : 0;
    applyDoState(index, value);

    if (!m_io || !m_io->isOpen())
        return;

    QString err;
    if (!m_io->writeDo(index, value, &err)) {
        m_status->setText(QStringLiteral("写 DO%1 失败: %2").arg(index + 1).arg(err));
        return;
    }
    m_status->setText(QStringLiteral("DO%1 (bit%2) = %3")
                          .arg(index + 1).arg(Tca9554::doBit(index)).arg(value));
}

void MainWindow::refreshIo()
{
    int i;
    const bool open = (m_io && m_io->isOpen());

    for (i = 0; i < Tca9554::kDiCount; ++i)
        setDiState(i, open ? m_io->readDi(i) : -1);

    if (open) {
        for (i = 0; i < Tca9554::kDoCount; ++i) {
            int v = m_io->readDo(i);
            if (v >= 0)
                applyDoState(i, v);
        }
        m_reg->setText(QStringLiteral("IN=0x%1   OUT=0x%2   CFG=0x%3")
                           .arg(m_io->readRegister(0x00), 2, 16, QChar('0'))
                           .arg(m_io->readRegister(0x01), 2, 16, QChar('0'))
                           .arg(m_io->readRegister(0x03), 2, 16, QChar('0'))
                           .toUpper());
    } else {
        m_reg->setText(QStringLiteral("IN=0x--   OUT=0x--   CFG=0x--"));
    }
}
