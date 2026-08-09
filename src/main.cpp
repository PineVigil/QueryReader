#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>

#include "ui/MainWindow.h"

#ifdef Q_OS_WIN
#include <windows.h>

static LONG WINAPI crashHandler(EXCEPTION_POINTERS *exInfo)
{
    if (exInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        QMessageBox::critical(nullptr, QStringLiteral("QueryReader 崩溃"),
                              QStringLiteral("程序遇到内存访问错误，无法继续。\n\n"
                                             "可能原因：文件格式不受支持或文件已损坏。"));
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(crashHandler);
#endif

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
