#ifndef TXTPARSER_H
#define TXTPARSER_H

#include <QFont>
#include <QStringList>

#include "parsers/IParser.h"

class TxtParser : public IParser
{
public:
    QStringList extensions() const override;
    QSharedPointer<Document> parse(const QString &filePath) override;
};

#endif // TXTPARSER_H