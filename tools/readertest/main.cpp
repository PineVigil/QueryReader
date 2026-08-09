// 临时验证工具：解析文档并导出各页为 PNG
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTextStream>
#include <QTimer>

#include "core/DocumentManager.h"
#include "core/MuPDFDocument.h"
#include "parsers/MarkdownParser.h"
#include "parsers/MuPDFParser.h"
#include "parsers/TxtParser.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const QStringList args = app.arguments();
    if (args.size() < 2) {
        qWarning() << "usage: ReaderTest <file>";
        return 1;
    }

    DocumentManager mgr;
    mgr.registerParser(new TxtParser());
    mgr.registerParser(new MarkdownParser());
    mgr.registerParser(new MuPDFParser());

    // 先用 MuPDFDocument 直连打开，失败时打印底层错误信息（仅对 MuPDF 扩展名）
    {
        const QString ext = QFileInfo(args.at(1)).suffix().toLower();
        if (MuPDFParser().extensions().contains(ext)) {
            MuPDFDocument probe(args.at(1), ext);
            if (!probe.isValid()) {
                qWarning() << "MuPDF probe failed:" << probe.lastError();
            }
        }
    }

    auto doc = mgr.openFile(args.at(1));
    if (!doc) {
        qWarning() << "FAILED to open:" << args.at(1);
        return 1;
    }

    qInfo() << "title:" << doc->title();
    qInfo() << "pages:" << doc->pageCount();

    // 结果写入文件（避免控制台重定向的刷新问题）
    QFile report(QDir::current().filePath("report.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "cannot open report";
        return 1;
    }
    QTextStream out(&report);
    auto trace = [&](const QString &line) {
        out << line << "\n";
        out.flush();
        qWarning().noquote() << "[TRACE]" << line;
    };

    trace(QStringLiteral("title: %1").arg(doc->title()));
    trace(QStringLiteral("pages: %1").arg(doc->pageCount()));
    trace(QStringLiteral("step-mgr-open-ok"));

    QDir outDir(QDir::current().filePath("page_export"));
    if (!outDir.exists()) {
        QDir().mkpath(outDir.absolutePath());
    }

    const int n = qMin(doc->pageCount(), 10);
    for (int i = 0; i < n; ++i) {
        QImage img = doc->renderPage(i, 1.0);
        trace(QStringLiteral("render %1 done w=%2 h=%3 null=%4").arg(i).arg(img.width()).arg(img.height()).arg(img.isNull()));
        const QString name = outDir.filePath(QStringLiteral("page_%1.png").arg(i + 1, 3, 10, QLatin1Char('0')));
        if (img.isNull()) {
            qWarning() << "page" << i << "render failed";
            trace(QStringLiteral("page %1 RENDER FAILED").arg(i));
            continue;
        }
        if (!img.save(name)) {
            qWarning() << "save failed:" << name;
            trace(QStringLiteral("page %1 SAVE FAILED").arg(i));
            return 1;
        }
        trace(QStringLiteral("page %1 saved").arg(i));
        const QString txt = doc->pageText(i);
        trace(QStringLiteral("page %1 textlen=%2").arg(i).arg(txt.size()));
    }

    {
        const auto outline = doc->outline();
        trace(QStringLiteral("outline count: %1").arg(outline.size()));
        for (const auto &oi : outline) {
            trace(QStringLiteral("  [%1] %2 -> page %3").arg(oi.level).arg(oi.title).arg(oi.page));
        }
    }
    trace(QStringLiteral("RENDER/TEXT/OUTLINE OK, idle-waiting 60s..."));
    QTimer::singleShot(60000, &app, &QCoreApplication::quit);
    const int rc = app.exec();
    trace(QStringLiteral("GUI-idle exited rc=%1").arg(rc));
    report.close();
    return rc;
}