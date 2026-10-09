#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("TCA9554 GPIO Test"));

    MainWindow window;
    window.show();
    return app.exec();
}
