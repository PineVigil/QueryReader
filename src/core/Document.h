#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <QString>
#include <QVector>

struct OutlineItem
{
    QString title;
    int page = 0;
    int level = 0;
};

class Document
{
public:
    virtual ~Document() = default;

    virtual QString title() const = 0;
    virtual int pageCount() const = 0;

    // 渲染指定页为图像。
    // scale: 缩放系数 (1.0 = 原始大小)
    virtual QImage renderPage(int page, double scale) const = 0;

    // 提取指定页的纯文本（用于搜索和复制）
    virtual QString pageText(int page) const = 0;

    // 文档目录 (PDF/EPUB 有，流式文档可能为空)
    virtual QVector<OutlineItem> outline() const { return {}; }
};

#endif // DOCUMENT_H