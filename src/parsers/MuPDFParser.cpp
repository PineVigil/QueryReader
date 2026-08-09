#include "parsers/MuPDFParser.h"

#include <QFileInfo>

#include "core/MuPDFDocument.h"

QStringList MuPDFParser::extensions() const
{
    // MuPDF 可识别的文档格式（txt/md 由专门的流式解析器处理，这里不重复注册）
    return {QStringLiteral("pdf"),  QStringLiteral("epub"), QStringLiteral("docx"),
            QStringLiteral("doc"),  QStringLiteral("xlsx"), QStringLiteral("pptx"),
            QStringLiteral("odt"),  QStringLiteral("ods"),  QStringLiteral("odp"),
            QStringLiteral("xps"),  QStringLiteral("cbz"),  QStringLiteral("cbr"),
            QStringLiteral("mobi"), QStringLiteral("fb2"),  QStringLiteral("svg"),
            QStringLiteral("png"),  QStringLiteral("jpg"),  QStringLiteral("jpeg"),
            QStringLiteral("gif"),  QStringLiteral("bmp"),  QStringLiteral("tif"),
            QStringLiteral("tiff")};
}

QSharedPointer<Document> MuPDFParser::parse(const QString &filePath)
{
    // 显式传 magic 让 MuPDF 选中正确的文档处理器（stream 方式打开时内容探测不完全可靠）
    const QString ext = QFileInfo(filePath).suffix().toLower();
    auto doc = QSharedPointer<MuPDFDocument>::create(filePath, ext);
    if (!doc->isValid()) {
        return nullptr;
    }
    return doc;
}