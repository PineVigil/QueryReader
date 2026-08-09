#include "core/TextDocument.h"

#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPalette>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>

namespace {

QFont pixelFont(const QFont &f)
{
    QFont nf = f;
    // pointSizeF -> 像素（96 DPI 假设；Qt 后续按 painter 缩放）
    nf.setPixelSize(qMax(4, static_cast<int>(f.pointSizeF() * 96.0 / 72.0)));
    return nf;
}

} // namespace

TextDocument::TextDocument(QString text, QString title,
                           QFont font, qreal pageWidth, qreal pageHeight,
                           bool htmlMode)
    : m_text(std::move(text))
    , m_title(std::move(title))
    , m_font(std::move(font))
    , m_pageWidth(pageWidth)
    , m_pageHeight(pageHeight)
    , m_htmlMode(htmlMode)
{
    rebuild();
}

QString TextDocument::title() const
{
    return m_title;
}

int TextDocument::pageCount() const
{
    return m_pages.size();
}

namespace {

struct LineInfo
{
    qreal top = 0;
    qreal bottom = 0;
    int charStart = 0; // 该行第一个字符在全文中的下标
    int charLen = 0;   // 该行字符数
};

} // namespace

// 强制排版：访问 documentSize() 触发完整布局，使 lineAt/lineCount 可用
static void forceLayout(QTextDocument &doc)
{
    (void)doc.documentLayout()->documentSize();
}

// 把主文档排版并返回全部视觉行
static QVector<LineInfo> layoutLines(const QString &text, const QFont &font,
                                     qreal pageWidth, bool htmlMode)
{
    QTextDocument doc;
    doc.setDefaultFont(pixelFont(font));
    if (htmlMode) {
        doc.setHtml(text);
    } else {
        doc.setPlainText(text);
    }
    doc.setTextWidth(pageWidth);
    forceLayout(doc);

    QVector<LineInfo> lines;
    QTextBlock block = doc.begin();
    for (; block.isValid(); block = block.next()) {
        QTextLayout *layout = block.layout();
        const qreal baseY = layout->position().y();
        const int n = layout->lineCount();
        for (int i = 0; i < n; ++i) {
            QTextLine line = layout->lineAt(i);
            LineInfo info;
            info.top = baseY + line.y();
            info.bottom = info.top + line.height();
            info.charStart = block.position() + line.textStart();
            info.charLen = line.textLength();
            lines.append(info);
        }
    }
    return lines;
}

void TextDocument::rebuild()
{
    m_pages.clear();

    const auto lines = layoutLines(m_text, m_font, m_pageWidth, m_htmlMode);

    int start = 0;
    while (start < lines.size()) {
        const qreal origin = lines[start].top;
        int end = start;
        while (end < lines.size() && lines[end].bottom <= origin + m_pageHeight + 0.01) {
            ++end;
        }
        if (end == start) {
            ++end; // 单行超高兜底，防止死循环
        }
        m_pages.append({origin, start, end - 1});
        start = end;
    }
}

QImage TextDocument::renderPage(int page, double scale) const
{
    if (page < 0 || page >= m_pages.size()) {
        return QImage();
    }

    const int w = static_cast<int>(m_pageWidth * scale);
    const int h = static_cast<int>(m_pageHeight * scale);
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    const Page &pg = m_pages[page];

    QPainter painter(&image);
    painter.scale(scale, scale);
    painter.translate(0, -pg.originY);
    painter.setClipRect(0, pg.originY, m_pageWidth, m_pageHeight);

    QTextDocument doc;
    doc.setDefaultFont(pixelFont(m_font));
    if (m_htmlMode) {
        doc.setHtml(m_text);
    } else {
        doc.setPlainText(m_text);
    }
    doc.setTextWidth(m_pageWidth);
    forceLayout(doc);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, Qt::black);
    doc.documentLayout()->draw(&painter, ctx);

    return image;
}

QString TextDocument::pageText(int page) const
{
    if (page < 0 || page >= m_pages.size()) {
        return QString();
    }

    if (m_htmlMode) {
        // HTML 模式下直接取该页可见的纯文本
        QTextDocument doc;
        doc.setHtml(m_text);
        doc.setTextWidth(m_pageWidth);
        forceLayout(doc);
        const Page &pg = m_pages[page];
        QTextBlock block = doc.begin();
        int current = -1;
        QString out;
        for (; block.isValid(); block = block.next()) {
            QTextLayout *layout = block.layout();
            const qreal baseY = layout->position().y();
            const int n = layout->lineCount();
            for (int i = 0; i < n; ++i) {
                QTextLine line = layout->lineAt(i);
                const qreal top = baseY + line.y();
                ++current;
                if (current < pg.firstLine) {
                    continue;
                }
                if (current > pg.lastLine) {
                    break;
                }
                if (!out.isEmpty()) {
                    out += QLatin1Char('\n');
                }
                out += block.text().mid(line.textStart(), line.textLength()).simplified();
            }
        }
        return out;
    }

    const auto lines = layoutLines(m_text, m_font, m_pageWidth, m_htmlMode);
    const Page &pg = m_pages[page];

    QString out;
    for (int i = pg.firstLine; i <= pg.lastLine; ++i) {
        const LineInfo &li = lines[i];
        if (!out.isEmpty()) {
            out += QLatin1Char('\n');
        }
        out += m_text.mid(li.charStart, li.charLen);
    }
    return out;
}