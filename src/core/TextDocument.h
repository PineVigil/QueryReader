#ifndef TEXTDOCUMENT_H
#define TEXTDOCUMENT_H

#include <QFont>
#include <QString>
#include <QVector>

#include "core/Document.h"

class QPainter;

// 流式文档（TXT / Markdown 等）：将纯文本排版后按页高切分成逻辑页。
// 整篇使用一个主 QTextDocument 统一排版，绘制时平移+裁剪，保证换行一致。
class TextDocument : public Document
{
public:
    TextDocument(QString text, QString title,
                 QFont font, qreal pageWidth, qreal pageHeight,
                 bool htmlMode = false);

    QString title() const override;
    int pageCount() const override;
    QImage renderPage(int page, double scale) const override;
    QString pageText(int page) const override;

private:
    struct Page
    {
        qreal originY = 0;   // 该页第一行在主排版中的纵向位置
        int firstLine = 0;   // 该页第一条视觉行下标
        int lastLine = 0;    // 该页最后一条视觉行下标（含）
    };

    void rebuild();

    QString m_text;
    QString m_title;
    QFont m_font;
    qreal m_pageWidth;
    qreal m_pageHeight;
    bool m_htmlMode = false;
    QVector<Page> m_pages;
};

#endif // TEXTDOCUMENT_H