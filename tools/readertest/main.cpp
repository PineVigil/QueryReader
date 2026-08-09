// 临时验证工具：解析文档并导出各页为 PNG
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTextStream>

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
    out << "title: " << doc->title() << "\n";
    out << "pages: " << doc->pageCount() << "\n";

    QDir outDir(QDir::current().filePath("page_export"));
    if (!outDir.exists()) {
        QDir().mkpath(outDir.absolutePath());
    }

    const int n = qMin(doc->pageCount(), 10);
    for (int i = 0; i < n; ++i) {
        QImage img = doc->renderPage(i, 1.0);
        const QString name = outDir.filePath(QStringLiteral("page_%1.png").arg(i + 1, 3, 10, QLatin1Char('0')));
        if (img.isNull()) {
            qWarning() << "page" << i << "render failed";
            out << "page " << i << ": RENDER FAILED\n";
            continue;
        }
        if (!img.save(name)) {
            qWarning() << "save failed:" << name;
            out << "page " << i << ": SAVE FAILED\n";
            return 1;
        }
        out << "page " << i << " saved " << name << " " << img.width() << "x" << img.height() << "\n";
        const QString txt = doc->pageText(i);
        out << "page " << i << " text(first 80): " << txt.left(80).replace(QLatin1Char('\n'), QLatin1Char(' ')) << "\n";
    }

    report.close();
    return 0;
}