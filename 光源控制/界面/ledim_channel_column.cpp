#include "ledim_channel_column.h"

#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#define LEDIM_GUI_settle_ms 20
#define LEDIM_GUI_max_level 255

LedimChannelColumn::LedimChannelColumn(int arg_channel_order, QWidget* arg_parent)
	: QWidget(arg_parent), m_channelOrder(arg_channel_order) {

	QFont tmp_label_font = font();
	tmp_label_font.setPointSizeF(font().pointSizeF() * 0.85);

	QLabel* tmp_caption = new QLabel(QStringLiteral("CH%1").arg(m_channelOrder), this);
	tmp_caption->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	tmp_caption->setFont(tmp_label_font);

	m_slider = new QSlider(Qt::Vertical, this);
	m_slider->setRange(0, LEDIM_GUI_max_level);
	m_slider->setValue(10); // the first generation tool started every channel at ten
	m_slider->setTickPosition(QSlider::NoTicks);
	m_slider->setTickInterval(25);
	m_slider->setMinimumHeight(120);

	m_levelEdit = new QLineEdit(QString::number(m_slider->value()), this);
	m_levelEdit->setReadOnly(true);
	m_levelEdit->setAlignment(Qt::AlignCenter);
	m_levelEdit->setMaximumWidth(44);
	m_levelEdit->setFont(tmp_label_font);

	m_turnOnButton = new QPushButton(QStringLiteral("开"), this);
	m_turnOffButton = new QPushButton(QStringLiteral("关"), this);
	m_readButton = new QPushButton(QStringLiteral("Read"), this);
	m_triggerButton = new QPushButton(QStringLiteral("触发"), this);
	m_modeButton = new QPushButton(QStringLiteral("模式"), this);

	QList<QPushButton*> tmp_buttons = {
		m_turnOnButton, m_turnOffButton, m_readButton, m_triggerButton, m_modeButton
	};
	for (QPushButton* tmp_button : tmp_buttons) {
		tmp_button->setMaximumWidth(44);
		tmp_button->setFont(tmp_label_font);
	}

	QVBoxLayout* tmp_layout = new QVBoxLayout(this);
	tmp_layout->setContentsMargins(2, 2, 2, 2);
	tmp_layout->setSpacing(3);
	tmp_layout->addWidget(tmp_caption);
	tmp_layout->addWidget(m_slider, 1, Qt::AlignHCenter);
	tmp_layout->addWidget(m_levelEdit, 0, Qt::AlignHCenter);
	tmp_layout->addWidget(m_turnOnButton);
	tmp_layout->addWidget(m_turnOffButton);
	tmp_layout->addWidget(m_readButton);
	tmp_layout->addWidget(m_triggerButton);
	tmp_layout->addWidget(m_modeButton);

	m_settleTimer = new QTimer(this);
	m_settleTimer->setSingleShot(true);
	m_settleTimer->setInterval(LEDIM_GUI_settle_ms);

	connect(m_slider, &QSlider::valueChanged, this, &LedimChannelColumn::onSliderMoved);
	connect(m_settleTimer, &QTimer::timeout, this, &LedimChannelColumn::onSliderSettled);
	connect(m_turnOnButton, &QPushButton::clicked, this, [this]() { emit turnOnRequested(m_channelOrder); });
	connect(m_turnOffButton, &QPushButton::clicked, this, [this]() { emit turnOffRequested(m_channelOrder); });
	connect(m_readButton, &QPushButton::clicked, this, [this]() { emit readRequested(m_channelOrder); });
	connect(m_triggerButton, &QPushButton::clicked, this, [this]() { emit triggerRequested(m_channelOrder); });
	connect(m_modeButton, &QPushButton::clicked, this, [this]() { emit modeRequested(m_channelOrder); });
}

void LedimChannelColumn::setLevel(int arg_level) {
	// the slider is the source of truth on screen, so a level coming from elsewhere only ever
	// fills the slider in; the settle timer stays out of it, so no frame is sent back out
	m_slider->blockSignals(true);
	m_slider->setValue(arg_level);
	m_slider->blockSignals(false);

	m_levelEdit->setText(QString::number(arg_level));
}

int LedimChannelColumn::level() const {
	return m_slider->value();
}

void LedimChannelColumn::setColumnEnabled(bool arg_enabled) {
	m_slider->setEnabled(arg_enabled);
	m_turnOnButton->setEnabled(arg_enabled);
	m_turnOffButton->setEnabled(arg_enabled);
	m_readButton->setEnabled(arg_enabled);
	m_triggerButton->setEnabled(arg_enabled);
	m_modeButton->setEnabled(arg_enabled);
}

void LedimChannelColumn::onSliderMoved(int arg_level) {
	m_levelEdit->setText(QString::number(arg_level));
	m_settleTimer->start();
}

void LedimChannelColumn::onSliderSettled() {
	emit levelChanged(m_channelOrder, m_slider->value());
}
