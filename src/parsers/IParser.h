#ifndef IPARSER_H
#define IPARSER_H

#include <QSharedPointer>
#include <QString>

#include "core/Document.h"

class IParser
{
public:
    virtual ~IParser() = default;

    // 返回该解析器支持的扩展名列表（不含点，小写），如 {"txt"} 或 {"pdf"}
    virtual QStringList extensions() const = 0;

    // 解析文件并返回 Document。失败时返回 nullptr。
    virtual QSharedPointer<Document> parse(const QString &filePath) = 0;
};

#endif // IPARSER_H