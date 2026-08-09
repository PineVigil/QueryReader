#ifndef DOCUMENTMANAGER_H
#define DOCUMENTMANAGER_H

#include <QHash>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

#include "core/Document.h"
#include "parsers/IParser.h"

class DocumentManager
{
public:
    DocumentManager();
    ~DocumentManager();

    void registerParser(IParser *parser);
    QSharedPointer<Document> openFile(const QString &filePath);
    QStringList supportedExtensions() const;
    QString supportedFilter() const;

private:
    QList<IParser *> m_parsers;
};

#endif // DOCUMENTMANAGER_H