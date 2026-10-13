/*
* libledim-gui, the window of the first generation tool, on top of the rewritten library
*
* usage: libledim-gui [-p portname] [--screenshot file.png]
*
*   -p, --port   connect to that port as soon as the window is up
*   --screenshot render the window into a png and exit, which is how the layout is checked without
*                a screen (it is meant to be used with QT_QPA_PLATFORM=offscreen)
*/

#include "ledim_main_window.h"
#include "ledim_mode_dialog.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QPixmap>
#include <QTimer>

int main(int argc, char* argv[]) {
	QApplication tmp_app(argc, argv);
	QApplication::setApplicationName(QStringLiteral("libledim-gui"));
	QApplication::setApplicationVersion(QStringLiteral("2.0.0"));

	QCommandLineParser tmp_parser;
	tmp_parser.setApplicationDescription(QStringLiteral("光源控制器图形界面 (libledim)"));
	tmp_parser.addHelpOption();
	tmp_parser.addVersionOption();

	QCommandLineOption tmp_port_option(
		QStringList() << QStringLiteral("p") << QStringLiteral("port"),
		QStringLiteral("要连接的串口, 例如 /dev/ttyUSB0"),
		QStringLiteral("portname")
	);
	tmp_parser.addOption(tmp_port_option);

	QCommandLineOption tmp_screenshot_option(
		QStringLiteral("screenshot"),
		QStringLiteral("把窗口渲染成 png 后退出 (布局自查用)"),
		QStringLiteral("file")
	);
	tmp_parser.addOption(tmp_screenshot_option);

	QCommandLineOption tmp_selfcheck_option(
		QStringLiteral("selfcheck"),
		QStringLiteral("连接后自动把 CH1 的按钮点一遍再退出 (接口接线自查用)")
	);
	tmp_parser.addOption(tmp_selfcheck_option);

	QCommandLineOption tmp_screenshot_mode_option(
		QStringLiteral("screenshot-mode"),
		QStringLiteral("把 \"模式\" 对话框渲染成 png 后退出 (布局自查用)"),
		QStringLiteral("file")
	);
	tmp_parser.addOption(tmp_screenshot_mode_option);

	tmp_parser.process(tmp_app);

	if (tmp_parser.isSet(tmp_screenshot_mode_option)) {
		LedimModeDialog tmp_dialog(1, 2, 50);
		tmp_dialog.show();
		const QString tmp_path = tmp_parser.value(tmp_screenshot_mode_option);
		QTimer::singleShot(300, &tmp_app, [&tmp_dialog, tmp_path]() {
			tmp_dialog.grab().save(tmp_path);
			QApplication::quit();
		});
		return tmp_app.exec();
	}

	LedimMainWindow tmp_window;
	tmp_window.show();

	if (tmp_parser.isSet(tmp_port_option)) {
		tmp_window.connectToPort(tmp_parser.value(tmp_port_option));
	}

	if (tmp_parser.isSet(tmp_selfcheck_option)) {
		tmp_window.runSelfCheck();
		QTimer::singleShot(600, &tmp_app, &QApplication::quit);
	}

	if (tmp_parser.isSet(tmp_screenshot_option)) {
		const QString tmp_path = tmp_parser.value(tmp_screenshot_option);
		// give the event loop a moment to lay the widgets out and to let a connect settle
		QTimer::singleShot(400, &tmp_app, [&tmp_window, tmp_path]() {
			if (tmp_window.grab().save(tmp_path)) {
				qInfo("窗口已渲染到 %s", qPrintable(tmp_path));
			} else {
				qWarning("无法写入 %s", qPrintable(tmp_path));
			}
			QApplication::quit();
		});
	}

	return tmp_app.exec();
}
