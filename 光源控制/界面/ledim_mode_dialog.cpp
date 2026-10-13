#include "ledim_mode_dialog.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

LedimModeDialog::LedimModeDialog(int arg_channel_order, int arg_current_mode, int arg_current_flash_ms, QWidget* arg_parent)
	: QDialog(arg_parent) {

	setWindowTitle(QStringLiteral("Set Mode"));

	m_normalRadio = new QRadioButton(QStringLiteral("常亮"), this);
	m_triggerRadio = new QRadioButton(QStringLiteral("外部触发"), this);
	m_flashRadio = new QRadioButton(QStringLiteral("频闪"), this);

	switch (arg_current_mode) {
	case 1:
		m_triggerRadio->setChecked(true);
		break;
	case 2:
		m_flashRadio->setChecked(true);
		break;
	default:
		m_normalRadio->setChecked(true);
		break;
	}

	QGroupBox* tmp_mode_box = new QGroupBox(QStringLiteral("通道 %1 的模式").arg(arg_channel_order), this);
	QVBoxLayout* tmp_mode_layout = new QVBoxLayout(tmp_mode_box);
	tmp_mode_layout->addWidget(m_normalRadio);
	tmp_mode_layout->addWidget(m_triggerRadio);
	tmp_mode_layout->addWidget(m_flashRadio);

	m_flashEdit = new QLineEdit(QString::number(arg_current_flash_ms), this);
	m_flashEdit->setValidator(new QIntValidator(0, 255, this));
	m_flashEdit->setFixedWidth(60);

	QLabel* tmp_unit_label = new QLabel(QStringLiteral("ms"), this);
	QHBoxLayout* tmp_time_row = new QHBoxLayout();
	tmp_time_row->addWidget(new QLabel(QStringLiteral("频闪时间:"), this));
	tmp_time_row->addWidget(m_flashEdit);
	tmp_time_row->addWidget(tmp_unit_label);
	tmp_time_row->addStretch(1);

	QDialogButtonBox* tmp_buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		this
	);
	tmp_buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
	tmp_buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

	QVBoxLayout* tmp_layout = new QVBoxLayout(this);
	tmp_layout->addWidget(tmp_mode_box);
	tmp_layout->addLayout(tmp_time_row);
	tmp_layout->addStretch(1);
	tmp_layout->addWidget(tmp_buttons);

	connect(tmp_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(tmp_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_normalRadio, &QRadioButton::toggled, this, &LedimModeDialog::onModeToggled);
	connect(m_triggerRadio, &QRadioButton::toggled, this, &LedimModeDialog::onModeToggled);
	connect(m_flashRadio, &QRadioButton::toggled, this, &LedimModeDialog::onModeToggled);

	onModeToggled();
}

void LedimModeDialog::onModeToggled() {
	// the time field only means anything in flash mode
	const bool tmp_is_flash = m_flashRadio->isChecked();
	m_flashEdit->setEnabled(tmp_is_flash);

	return;
}

int LedimModeDialog::selectedMode() const {
	if (m_triggerRadio->isChecked()) {
		return 1;
	}
	if (m_flashRadio->isChecked()) {
		return 2;
	}

	return 0;
}

int LedimModeDialog::flashDurationMs() const {
	return m_flashEdit->text().toInt();
}
