#include "parsers/TxtParser.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include "core/TextDocument.h"

namespace {

QString detectEncoding(QFile &file)
{
    // 读取 BOM 判断编码
    const QByteArray head = file.peek(3);
    if (head.startsWith("\xEF\xBB\xBF")) {
        return "UTF-8";
    }
    if (head.startsWith("\xFF\xFE")) {
        return "UTF-16LE";
    }
    if (head.startsWith("\xFE\xFF")) {
        return "UTF-16BE";
    }
    return "UTF-8"; // 默认 UTF-8（无 BOM）
}

} // namespace

QStringList TxtParser::extensions() const
{
    return {QStringLiteral("txt"), QStringLiteral("text"), QStringLiteral("log"),
            QStringLiteral("ini"), QStringLiteral("cfg"), QStringLiteral("cs"),
            QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("hpp"),
            QStringLiteral("java"), QStringLiteral("py"), QStringLiteral("js"),
            QStringLiteral("html"), QStringLiteral("css"), QStringLiteral("json")};
}

QSharedPointer<Document> TxtParser::parse(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return nullptr;
    }

    const QString encoding = detectEncoding(file);
    QTextStream stream(&file);
    const QByteArray encName = encoding.toLatin1();
    auto conv = QStringConverter::encodingForName(encName.constData());
    if (conv) {
        stream.setEncoding(*conv);
    } else {
        stream.setEncoding(QStringConverter::Utf8);
    }
    const QString text = stream.readAll();
    file.close();

    QFont defaultFont(QStringLiteral("Microsoft YaHei"));
    defaultFont.setPointSizeF(11.0);
    const double pageW = 595.0; // A4 宽度（pt），后续可由 UI 设置
    const double pageH = 842.0; // A4 高度（pt）

    auto doc = QSharedPointer<TextDocument>::create(
        text,
        QFileInfo(filePath).completeBaseName(),
        defaultFont,
        pageW,
        pageH);

    return doc;
}