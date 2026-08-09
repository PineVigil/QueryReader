#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QStringList>

#include "parsers/IParser.h"

class MarkdownParser : public IParser
{
public:
    QStringList extensions() const override;
    QSharedPointer<Document> parse(const QString &filePath) override;
};

#endif // MARKDOWNPARSER_H