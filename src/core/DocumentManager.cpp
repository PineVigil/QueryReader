#include "core/DocumentManager.h"

#include <QFileInfo>
#include <QString>

DocumentManager::DocumentManager() = default;

DocumentManager::~DocumentManager()
{
    qDeleteAll(m_parsers);
}

void DocumentManager::registerParser(IParser *parser)
{
    if (parser && !m_parsers.contains(parser)) {
        m_parsers.append(parser);
    }
}

QSharedPointer<Document> DocumentManager::openFile(const QString &filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    for (IParser *parser : m_parsers) {
        if (parser->extensions().contains(ext)) {
            return parser->parse(filePath);
        }
    }
    return nullptr;
}

QStringList DocumentManager::supportedExtensions() const
{
    QStringList exts;
    for (IParser *parser : m_parsers) {
        exts.append(parser->extensions());
    }
    exts.removeDuplicates();
    return exts;
}

QString DocumentManager::supportedFilter() const
{
    const QStringList exts = supportedExtensions();
    QStringList patterns;
    for (const QString &e : exts) {
        patterns.append(QStringLiteral("*.%1").arg(e));
    }
    return QStringLiteral("所有支持文档 (%1);;所有文件 (*)").arg(patterns.join(QLatin1Char(' ')));
}