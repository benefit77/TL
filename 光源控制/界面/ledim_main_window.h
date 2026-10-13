#ifndef __ledim_main_window_h
#define __ledim_main_window_h

#include <QStringList>
#include <QWidget>

#include <libledim.h>

class LedimChannelColumn;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;

/*
* the window of the first generation tool, rebuilt on top of libledim
*
* what it kept from that tool: the serial port row (a port to pick, a search button, connect and
* release buttons), the strip of channel columns (four wired channels and four greyed out ones,
* since the controller has four), the per channel slider / value / 开 / 关 / Read / 触发 / 模式
* buttons, and the log pane on the right with the protocol hint above it.
*
* what it does differently, because the library can tell: a refused command, a port that could not
* be opened and a controller that stopped answering are reported with the reason, not with a
* number, and the port list holds the names the host actually uses (/dev/ttyUSB0, COM3, ...).
*/
class LedimMainWindow : public QWidget {
	Q_OBJECT

public:
	explicit LedimMainWindow(QWidget* arg_parent = nullptr);

	// hand the library's own log lines to the pane, alongside the tool's own messages
	void appendLog(const QString& arg_line);

	// pick a port from the list and connect to it (used by the -p command line option)
	bool connectToPort(const QString& arg_port_name);

	/*
	* press the buttons of channel one the way an operator would, in order, so that the wiring
	* behind every control can be checked without a mouse (checked by the gui smoke test)
	*/
	void runSelfCheck();

private slots:
	void onSearchPorts();
	void onConnect();
	void onDisconnect();

	void onLevelChanged(int arg_channel_order, int arg_level);
	void onTurnOn(int arg_channel_order);
	void onTurnOff(int arg_channel_order);
	void onReadLevel(int arg_channel_order);
	void onTriggerFlash(int arg_channel_order);
	void onSetMode(int arg_channel_order);

private:
	bool isConnected() const;

	// the library logs through a plain C callback, these two give it somewhere to land
	static void logBridge(libledim_LogLevel arg_level, const char* arg_line);
	static LedimMainWindow* _fstatic_instance;

	QComboBox* m_portCombo;
	QPushButton* m_searchButton;
	QPushButton* m_connectButton;
	QPushButton* m_disconnectButton;
	QLabel* m_lastFrameLabel;
	QPlainTextEdit* m_logPane;

	QList<LedimChannelColumn*> m_columns;

	// what the operator last asked each channel for, so the mode box opens where it was left
	int m_channelModes[libledim_MG_max_num_channels];
	int m_channelFlashMs[libledim_MG_max_num_channels];

	// what the log pane is trimmed to, the same ceiling the first generation tool used
	int m_logLineLimit;
};

#endif // __ledim_main_window_h
