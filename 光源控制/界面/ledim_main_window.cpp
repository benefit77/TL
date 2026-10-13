#include "ledim_main_window.h"

#include "ledim_channel_column.h"
#include "ledim_mode_dialog.h"

#include <libledim.h>

#include <QComboBox>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#define LEDIM_GUI_log_line_limit 350

LedimMainWindow* LedimMainWindow::_fstatic_instance = nullptr;

static QString __lastErrorText() {
	QString tmp_text = QString::fromUtf8(libledim_GetLastErrorString());
	const int tmp_os_error = libledim_GetLastOsError();
	if (tmp_os_error != 0) {
		// the host level code is what turns "could not be opened" into an actionable message
		tmp_text += QStringLiteral(" (os error %1)").arg(tmp_os_error);
	}

	return tmp_text;
}

LedimMainWindow::LedimMainWindow(QWidget* arg_parent)
	: QWidget(arg_parent), m_logLineLimit(LEDIM_GUI_log_line_limit) {

	_fstatic_instance = this;
	for (int tmp_i = 0; tmp_i < libledim_MG_max_num_channels; tmp_i++) {
		m_channelModes[tmp_i] = 0; // normal, the mode every channel comes up in
		m_channelFlashMs[tmp_i] = 10; // the first generation tool started at ten milliseconds
	}
	setWindowTitle(QStringLiteral("PCSoftware - 光源控制器 (libledim)"));

	// ---------------------------------------------------------------------------------------
	// the serial port row: a port to pick, and the three buttons of the first generation tool
	// ---------------------------------------------------------------------------------------
	m_portCombo = new QComboBox(this);
	m_portCombo->setMinimumWidth(170);
	m_portCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	m_searchButton = new QPushButton(QStringLiteral("搜索控制器"), this);
	m_connectButton = new QPushButton(QStringLiteral("连接控制器"), this);
	m_disconnectButton = new QPushButton(QStringLiteral("关闭控制器"), this);

	QHBoxLayout* tmp_port_row = new QHBoxLayout();
	tmp_port_row->addWidget(new QLabel(QStringLiteral("串口号:"), this));
	tmp_port_row->addWidget(m_portCombo, 1);
	tmp_port_row->addWidget(m_searchButton);

	QHBoxLayout* tmp_connect_row = new QHBoxLayout();
	tmp_connect_row->addWidget(m_connectButton);
	tmp_connect_row->addWidget(m_disconnectButton);
	tmp_connect_row->addStretch(1);

	// ---------------------------------------------------------------------------------------
	// the channel strip
	// ---------------------------------------------------------------------------------------
	// the first generation tool drew an empty group box around the strip, this is the same frame
	QFrame* tmp_channels_box = new QFrame(this);
	tmp_channels_box->setObjectName(QStringLiteral("channelStrip"));
	tmp_channels_box->setFrameShape(QFrame::StyledPanel);
	tmp_channels_box->setFrameShadow(QFrame::Plain);
	// QLabel descends from QFrame, hence the object name in the selector: only the strip is boxed
	tmp_channels_box->setStyleSheet(
		QStringLiteral("QFrame#channelStrip { border: 1px solid palette(mid); }")
	);
	QHBoxLayout* tmp_channels_layout = new QHBoxLayout(tmp_channels_box);
	tmp_channels_layout->setContentsMargins(6, 6, 6, 6);
	tmp_channels_layout->setSpacing(4);

	for (int tmp_channel = 1; tmp_channel <= 8; tmp_channel++) {
		LedimChannelColumn* tmp_column = new LedimChannelColumn(tmp_channel, tmp_channels_box);
		// the controller has four channels, so five through eight are shown but not operable, which
		// is exactly what the first generation tool did
		tmp_column->setColumnEnabled(tmp_channel <= libledim_MG_max_num_channels);
		tmp_channels_layout->addWidget(tmp_column);
		m_columns.append(tmp_column);

		connect(tmp_column, &LedimChannelColumn::levelChanged, this, &LedimMainWindow::onLevelChanged);
		connect(tmp_column, &LedimChannelColumn::turnOnRequested, this, &LedimMainWindow::onTurnOn);
		connect(tmp_column, &LedimChannelColumn::turnOffRequested, this, &LedimMainWindow::onTurnOff);
		connect(tmp_column, &LedimChannelColumn::readRequested, this, &LedimMainWindow::onReadLevel);
		connect(tmp_column, &LedimChannelColumn::triggerRequested, this, &LedimMainWindow::onTriggerFlash);
		connect(tmp_column, &LedimChannelColumn::modeRequested, this, &LedimMainWindow::onSetMode);
	}

	// ---------------------------------------------------------------------------------------
	// the log pane, with the protocol hint the first generation tool put above it
	// ---------------------------------------------------------------------------------------
	m_lastFrameLabel = new QLabel(QStringLiteral("协议范文:$310381E"), this);
	m_lastFrameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_lastFrameLabel->setToolTip(QStringLiteral("控制器收到的最后一帧请求"));

	m_logPane = new QPlainTextEdit(this);
	m_logPane->setReadOnly(true);
	m_logPane->setMinimumWidth(320);
	m_logPane->setMaximumBlockCount(m_logLineLimit);
	// the first generation tool put a plain multiline text box here, so nothing scrolls out of
	// sight sideways when a message is longer than the pane
	m_logPane->setLineWrapMode(QPlainTextEdit::WidgetWidth);

	QFont tmp_mono_font = m_logPane->font();
	tmp_mono_font.setStyleHint(QFont::Monospace);
	tmp_mono_font.setFamily(QStringLiteral("monospace"));
	tmp_mono_font.setPointSizeF(tmp_mono_font.pointSizeF() * 0.95);
	m_logPane->setFont(tmp_mono_font);

	QVBoxLayout* tmp_log_layout = new QVBoxLayout();
	tmp_log_layout->addWidget(m_lastFrameLabel);
	tmp_log_layout->addWidget(m_logPane, 1);

	// ---------------------------------------------------------------------------------------
	// the whole window: the buttons and the channel strip on the left, the log on the right
	// ---------------------------------------------------------------------------------------
	QVBoxLayout* tmp_left_layout = new QVBoxLayout();
	tmp_left_layout->addLayout(tmp_port_row);
	tmp_left_layout->addLayout(tmp_connect_row);
	tmp_left_layout->addWidget(tmp_channels_box, 1);
	tmp_left_layout->addStretch(0);

	QHBoxLayout* tmp_layout = new QHBoxLayout(this);
	tmp_layout->addLayout(tmp_left_layout, 1);
	tmp_layout->addLayout(tmp_log_layout, 1);

	connect(m_searchButton, &QPushButton::clicked, this, &LedimMainWindow::onSearchPorts);
	connect(m_connectButton, &QPushButton::clicked, this, &LedimMainWindow::onConnect);
	connect(m_disconnectButton, &QPushButton::clicked, this, &LedimMainWindow::onDisconnect);

	// the library narrates what it does, the pane is where it lands
	libledim_SetLogCallback(&LedimMainWindow::logBridge);
	libledim_SetMinLogLevel(libledim_LogLevel_Debug);

	appendLog(QStringLiteral("libledim %1 就绪, 请先搜索并连接控制器").arg(QStringLiteral("2.0.0")));
	onSearchPorts();

	resize(1000, 580);
}

void LedimMainWindow::runSelfCheck() {
	appendLog(QStringLiteral("自检: 依次操作 CH1 (滑块 120 -> 开 -> Read -> 触发 -> 关)"));

	LedimChannelColumn* tmp_column = m_columns.at(0);
	QSlider* tmp_slider = tmp_column->findChild<QSlider*>();
	if (tmp_slider != nullptr) {
		tmp_slider->setValue(120); // the slider is the only source the column listens to
	}

	// the column holds the level back for 20ms before handing it to the device, so the buttons of
	// the sequence are pressed once the event loop has had a chance to run
	QTimer::singleShot(100, this, [this, tmp_column]() {
		const QStringList tmp_sequence = {
			QStringLiteral("开"), QStringLiteral("Read"), QStringLiteral("触发"), QStringLiteral("关")
		};
		for (const QString& tmp_caption : tmp_sequence) {
			for (QPushButton* tmp_button : tmp_column->findChildren<QPushButton*>()) {
				if (tmp_button->text() == tmp_caption) {
					tmp_button->click();
					break;
				}
			}
		}
		appendLog(QStringLiteral("自检: 完成"));
	});

	return;
}

void LedimMainWindow::appendLog(const QString& arg_line) {
	m_logPane->appendPlainText(arg_line);

	// keep the frame the controller saw last visible, the way the protocol hint of the first
	// generation tool invited one to read it
	if (arg_line.startsWith(QStringLiteral("D>  request: "))) {
		m_lastFrameLabel->setText(arg_line.mid(QStringLiteral("D>  request: ").size()).trimmed());
	}

	return;
}

void LedimMainWindow::logBridge(libledim_LogLevel arg_level, const char* arg_line) {
	(void)arg_level;
	if (_fstatic_instance == nullptr) {
		return;
	}
	// every library call is synchronous, so this lands on the window's own thread
	_fstatic_instance->appendLog(QString::fromUtf8(arg_line).trimmed());

	return;
}

bool LedimMainWindow::isConnected() const {
	return (libledim_GetComPortname() != nullptr);
}

void LedimMainWindow::onSearchPorts() {
	m_portCombo->clear();

	const uint32_t tmp_num_ports = libledim_CountComPorts();
	for (uint32_t tmp_i = 0; tmp_i < tmp_num_ports; tmp_i++) {
		m_portCombo->addItem(QString::fromUtf8(libledim_PeekComPortByIdx(tmp_i)));
	}

	appendLog(QStringLiteral("搜索控制器: 找到 %1 个串口").arg(tmp_num_ports));

	return;
}

void LedimMainWindow::onConnect() {
	if (isConnected()) {
		appendLog(QStringLiteral("控制器已经连接, 如需换一个请先关闭"));
		return;
	}

	if (m_portCombo->count() == 0) {
		appendLog(QStringLiteral("尚未选择控制器: 串口列表是空的, 请先搜索"));
		return;
	}

	const QString tmp_port_name = m_portCombo->currentText();
	if (!libledim_Init(tmp_port_name.toUtf8().constData())) {
		appendLog(QStringLiteral("连接控制器失败: %1").arg(__lastErrorText()));
		return;
	}

	appendLog(QStringLiteral("连接控制器: %1").arg(tmp_port_name));

	return;
}

bool LedimMainWindow::connectToPort(const QString& arg_port_name) {
	if (!m_portCombo->count()) {
		onSearchPorts();
	}

	const int tmp_index = m_portCombo->findText(arg_port_name);
	if (tmp_index >= 0) {
		m_portCombo->setCurrentIndex(tmp_index);
	} else {
		m_portCombo->addItem(arg_port_name);
		m_portCombo->setCurrentIndex(m_portCombo->count() - 1);
	}

	if (isConnected()) {
		return true;
	}
	if (!libledim_Init(arg_port_name.toUtf8().constData())) {
		appendLog(QStringLiteral("连接控制器失败: %1").arg(__lastErrorText()));
		return false;
	}

	appendLog(QStringLiteral("连接控制器: %1").arg(arg_port_name));

	return true;
}

void LedimMainWindow::onDisconnect() {
	if (!isConnected()) {
		appendLog(QStringLiteral("尚未选择控制器."));
		return;
	}

	libledim_Deinit();
	appendLog(QStringLiteral("关闭控制器: 串口已释放"));

	return;
}

void LedimMainWindow::onLevelChanged(int arg_channel_order, int arg_level) {
	if (!isConnected()) {
		appendLog(QStringLiteral("CH%1 亮度未下发: 尚未选择控制器.").arg(arg_channel_order));
		return;
	}

	if (!libledim_SetChannelLuminanceLevel((uint8_t)arg_channel_order, (uint8_t)arg_level)) {
		appendLog(QStringLiteral("CH%1 设置亮度 %2 失败: %3")
			.arg(arg_channel_order)
			.arg(arg_level)
			.arg(__lastErrorText()));
	}

	return;
}

void LedimMainWindow::onTurnOn(int arg_channel_order) {
	if (!isConnected()) {
		appendLog(QStringLiteral("尚未选择控制器."));
		return;
	}

	if (!libledim_EndisChannel((uint8_t)arg_channel_order, true)) {
		appendLog(QStringLiteral("CH%1 开 失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
	} else {
		appendLog(QStringLiteral("CH%1 开").arg(arg_channel_order));
	}

	return;
}

void LedimMainWindow::onTurnOff(int arg_channel_order) {
	if (!isConnected()) {
		appendLog(QStringLiteral("尚未选择控制器."));
		return;
	}

	if (!libledim_EndisChannel((uint8_t)arg_channel_order, false)) {
		appendLog(QStringLiteral("CH%1 关 失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
	} else {
		appendLog(QStringLiteral("CH%1 关").arg(arg_channel_order));
	}

	return;
}

void LedimMainWindow::onReadLevel(int arg_channel_order) {
	if (!isConnected()) {
		appendLog(QStringLiteral("尚未选择控制器."));
		return;
	}

	const uint8_t tmp_level = libledim_GetChannelLuminanceLevel((uint8_t)arg_channel_order);
	if (libledim_GetLastError() != libledim_Error_None) {
		appendLog(QStringLiteral("CH%1 读回亮度失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
		return;
	}

	m_columns.at(arg_channel_order - 1)->setLevel(tmp_level);
	appendLog(QStringLiteral("CH%1 亮度 = %2").arg(arg_channel_order).arg(tmp_level));

	return;
}

void LedimMainWindow::onTriggerFlash(int arg_channel_order) {
	if (!isConnected()) {
		appendLog(QStringLiteral("尚未选择控制器."));
		return;
	}

	if (!libledim_TriggerChannelFlash((uint8_t)arg_channel_order)) {
		appendLog(QStringLiteral("CH%1 触发失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
	} else {
		appendLog(QStringLiteral("CH%1 触发").arg(arg_channel_order));
	}

	return;
}

void LedimMainWindow::onSetMode(int arg_channel_order) {
	// the first generation tool opened the mode box first and complained about the missing handle
	// only when its ok button was pressed, which is what this does too
	LedimModeDialog tmp_dialog(
		arg_channel_order,
		m_channelModes[arg_channel_order - 1],
		m_channelFlashMs[arg_channel_order - 1],
		this
	);
	if (tmp_dialog.exec() != QDialog::Accepted) {
		return;
	}

	if (!isConnected()) {
		appendLog(QStringLiteral("操作句柄为空!"));
		return;
	}

	const int tmp_mode = tmp_dialog.selectedMode();
	const int tmp_flash_ms = tmp_dialog.flashDurationMs();
	m_channelModes[arg_channel_order - 1] = tmp_mode;
	m_channelFlashMs[arg_channel_order - 1] = tmp_flash_ms;

	if (!libledim_SetChannelMode((uint8_t)arg_channel_order, (uint8_t)tmp_mode)) {
		appendLog(QStringLiteral("CH%1 设置模式失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
		return;
	}
	if (tmp_mode == 2 && !libledim_SetChannelFlashDuration((uint8_t)arg_channel_order, (uint8_t)tmp_flash_ms)) {
		appendLog(QStringLiteral("CH%1 设置频闪时间失败: %2").arg(arg_channel_order).arg(__lastErrorText()));
		return;
	}

	appendLog(
		QStringLiteral("CH%1 模式 = %2%3")
			.arg(arg_channel_order)
			.arg(tmp_mode)
			.arg(tmp_mode == 2 ? QStringLiteral(", 频闪 %1 ms").arg(tmp_flash_ms) : QString())
	);

	return;
}
