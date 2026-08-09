#include "parsers/MarkdownParser.h"

#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <cmark-gfm.h>

#include "core/TextDocument.h"

namespace {

// cmark 输出 HTML，转换为 Qt QTextDocument 可接受的 HTML（补全头部/样式）
QString markdownToHtml(const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    char *html = cmark_markdown_to_html(utf8.constData(), utf8.size(), CMARK_OPT_UNSAFE);
    if (!html) {
        return QString();
    }
    QString result = QString::fromUtf8(html);
    free(html);

    // 让 QTextDocument 的 HTML 渲染更符合阅读习惯：
    // 段落间距、标题、引用、代码块等基础样式由 Qt 默认支持，
    // 这里仅设置默认字体与正文行高，其他交给 Qt 内建样式。
    return QStringLiteral(
        "<html><body style=\"font-family:'Microsoft YaHei'; font-size:11pt;\">%1</body></html>")
        .arg(result);
}

} // namespace

QStringList MarkdownParser::extensions() const
{
    return {QStringLiteral("md"), QStringLiteral("markdown"), QStringLiteral("mdown"),
            QStringLiteral("mkd"), QStringLiteral("mdt")};
}

QSharedPointer<Document> MarkdownParser::parse(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return nullptr;
    }
    const QString raw = QString::fromUtf8(file.readAll());
    file.close();

    const QString html = markdownToHtml(raw);
    if (html.isEmpty()) {
        return nullptr;
    }

    QFont defaultFont(QStringLiteral("Microsoft YaHei"));
    defaultFont.setPointSizeF(11.0);
    const double pageW = 595.0;
    const double pageH = 842.0;

    auto doc = QSharedPointer<TextDocument>::create(
        html,
        QFileInfo(filePath).completeBaseName(),
        defaultFont,
        pageW,
        pageH,
        true /* htmlMode */);

    return doc;
}