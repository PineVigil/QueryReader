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
    const DWORD code = exInfo->ExceptionRecord->ExceptionCode;
    QString detail;
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        detail = QStringLiteral("内存访问错误 (ACCESS_VIOLATION)");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        detail = QStringLiteral("栈溢出 (STACK_OVERFLOW)");
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        detail = QStringLiteral("非法指令 (ILLEGAL_INSTRUCTION)");
        break;
    default:
        detail = QStringLiteral("异常代码: 0x%1").arg(code, 0, 16);
    }
    QMessageBox::critical(
        nullptr, QStringLiteral("QueryReader 崩溃"),
        QStringLiteral("程序遇到严重错误，无法继续。\n\n%1\n\n"
                       "可能原因：文件格式不受支持或文件已损坏。")
            .arg(detail));
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstdlib>

static void sigHandler(int)
{
    QMessageBox::critical(nullptr, QStringLiteral("QueryReader 崩溃"),
                          QStringLiteral("程序遇到严重错误（段错误），无法继续。\n\n"
                                         "可能原因：文件格式不受支持或文件已损坏。"));
    _exit(1);
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(crashHandler);
#endif
#ifdef Q_OS_UNIX
    std::signal(SIGSEGV, sigHandler);
    std::signal(SIGBUS, sigHandler);
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
