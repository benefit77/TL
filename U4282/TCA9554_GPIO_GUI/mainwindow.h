#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>

class QLabel;
class QPushButton;
class QTimer;
class Tca9554;

class MainWindow : public QDialog
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onDoClicked();     // DO 值框被点击 -> 取反并写入
    void onAllOff();        // 全部输出清零
    void refreshIo();       // 定时刷新 DI / DO 回读 / 寄存器

private:
    void buildUi();
    void connectDevice();
    void writeDo(int index, int value);
    void applyDoState(int index, int value);    // 更新 DO 框(值 + 颜色)
    void setDiState(int index, int value);      // 更新 DI 框(值 + 颜色)
    void setConnected(bool ok);

    Tca9554     *m_io = nullptr;
    QLabel      *m_conn = nullptr;             // ● 已连接 · 地址
    QLabel      *m_status = nullptr;           // 最近一次操作提示
    QPushButton *m_doBox[4] = { nullptr, nullptr, nullptr, nullptr };  // DO 值框(点击取反)
    QLabel      *m_diBox[4] = { nullptr, nullptr, nullptr, nullptr };  // DI 值框(只读)
    QLabel      *m_reg = nullptr;              // IN/OUT/CFG 十六进制
    QPushButton *m_allOff = nullptr;
    QTimer      *m_timer = nullptr;
};

#endif // MAINWINDOW_H
