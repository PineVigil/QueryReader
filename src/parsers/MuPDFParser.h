#ifndef MUPDFPARSER_H
#define MUPDFPARSER_H

#include <QSharedPointer>
#include <QStringList>

#include "parsers/IParser.h"

// 基于 MuPDF 的通用解析器：PDF、EPUB、Office、XPS、CBZ、MOBI、FB2、SVG、图片等。
class MuPDFParser : public IParser
{
public:
    QStringList extensions() const override;
    QSharedPointer<Document> parse(const QString &filePath) override;
};

#endif // MUPDFPARSER_H