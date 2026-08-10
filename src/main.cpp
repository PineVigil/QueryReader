#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>

#include "ui/MainWindow.h"

#ifdef Q_OS_WIN
#include <windows.h>

static void writeCrashLog(const char *type, DWORD code = 0)
{
    QFile f(QCoreApplication::applicationDirPath() + QStringLiteral("/crash.log"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        f.write(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")).toUtf8());
        f.write(" CRASH: ");
        f.write(type);
        if (code) {
            f.write(QString(" (code 0x%1)").arg(code, 0, 16).toUtf8());
        }
        f.write("\n");
        f.close();
    }
}

static LONG WINAPI crashHandler(EXCEPTION_POINTERS *exInfo)
{
    const DWORD code = exInfo->ExceptionRecord->ExceptionCode;
    writeCrashLog("WindowsStructuredException", code);

    QString detail;
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        detail = QStringLiteral("内存访问错误 (ACCESS_VIOLATION)\n\n"
                                "可能原因：文件格式不受支持或文件已损坏。");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        detail = QStringLiteral("栈溢出 (STACK_OVERFLOW)");
        break;
    default:
        detail = QStringLiteral("异常代码: 0x%1").arg(code, 0, 16);
    }
    QMessageBox::critical(nullptr, QStringLiteral("QueryReader 崩溃"),
                          QStringLiteral("程序遇到严重错误，无法继续。\n\n%1").arg(detail));
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstdlib>

static void sigHandler(int sig)
{
    QFile f(QCoreApplication::applicationDirPath() + QStringLiteral("/crash.log"));
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        f.write(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")).toUtf8());
        f.write(QString(" CRASH: signal %1\n").arg(sig).toUtf8());
        f.close();
    }
    QMessageBox::critical(nullptr, QStringLiteral("QueryReader 崩溃"),
                          QStringLiteral("程序遇到严重错误，无法继续。\n\n"
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
    app.setApplicationVersion("1.1.0");

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
