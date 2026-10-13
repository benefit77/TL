#ifndef __ledim_channel_column_h
#define __ledim_channel_column_h

#include <QWidget>

class QLineEdit;
class QPushButton;
class QSlider;
class QTimer;

/*
* one column of the channel strip: the slider, the value read out, and the buttons of a channel
*
* this is the column that the first generation tool laid out by hand eight times (five through
* eight being grayed out, since the controller only has four channels). the column knows nothing
* about the controller: it says what the operator asked for and waits to be told what the level is.
*/
class LedimChannelColumn : public QWidget {
	Q_OBJECT

public:
	explicit LedimChannelColumn(int arg_channel_order, QWidget* arg_parent = nullptr);

	// show a level without telling the controller about it (a read back, or the startup state)
	void setLevel(int arg_level);
	int level() const;

	// whether the column can be operated at all (five through eight are not wired to anything)
	void setColumnEnabled(bool arg_enabled);

signals:
	void levelChanged(int arg_channel_order, int arg_level);
	void turnOnRequested(int arg_channel_order);
	void turnOffRequested(int arg_channel_order);
	void readRequested(int arg_channel_order);
	void triggerRequested(int arg_channel_order);
	void modeRequested(int arg_channel_order);

private slots:
	void onSliderMoved(int arg_level);
	void onSliderSettled();

private:
	int m_channelOrder;
	QSlider* m_slider;
	QLineEdit* m_levelEdit;
	QPushButton* m_turnOnButton;
	QPushButton* m_turnOffButton;
	QPushButton* m_readButton;
	QPushButton* m_triggerButton;
	QPushButton* m_modeButton;

	/*
	* the first generation tool polled the slider every 20ms and sent whatever it found, so a drag
	* went out as a stream of small frames. this timer keeps that cadence (a level reaches the
	* controller while the operator is still dragging) without sending one frame per pixel.
	*/
	QTimer* m_settleTimer;
};

#endif // __ledim_channel_column_h
