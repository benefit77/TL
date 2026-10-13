#ifndef __ledim_mode_dialog_h
#define __ledim_mode_dialog_h

#include <QDialog>

class QLineEdit;
class QRadioButton;

/*
* the "模式" dialog of a channel, the shape the first generation tool used for its "Set Mode" box:
* a group of radio buttons, a time field with its unit next to it, and an ok/cancel pair
*
* the fourth radio of that tool ("微秒级频闪") has no counterpart here: libledim sets the flash
* duration in milliseconds, and only in milliseconds, so offering a microsecond mode would be a
* control that cannot do anything.
*/
class LedimModeDialog : public QDialog {
	Q_OBJECT

public:
	LedimModeDialog(int arg_channel_order, int arg_current_mode, int arg_current_flash_ms, QWidget* arg_parent = nullptr);

	// 0 normal (always on), 1 external trigger, 2 flash
	int selectedMode() const;
	int flashDurationMs() const;

private slots:
	void onModeToggled();

private:
	QRadioButton* m_normalRadio;
	QRadioButton* m_triggerRadio;
	QRadioButton* m_flashRadio;
	QLineEdit* m_flashEdit;
};

#endif // __ledim_mode_dialog_h
