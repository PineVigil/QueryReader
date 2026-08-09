#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QString>

#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("QueryReader");
    app.setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("QueryReader - 轻量阅读器");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "要打开的文档路径");
    parser.process(app);

    MainWindow window;
    window.show();

    const QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QFileInfo fi(args.first());
        if (fi.isFile()) {
            window.openPath(args.first());
        }
    }

    return app.exec();
}