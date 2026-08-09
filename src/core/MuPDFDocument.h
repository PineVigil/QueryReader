#ifndef MUPDFDOCUMENT_H
#define MUPDFDOCUMENT_H

#include <QString>
#include <QVector>

#include "core/Document.h"

struct fz_context;
struct fz_stream;
struct fz_document;

// 基于 MuPDF 的通用文档：支持 PDF、EPUB、Office、XPS、CBZ、MOBI 等 MuPDF 可识别的格式。
class MuPDFDocument : public Document
{
public:
    // magic: 格式标识（如 "pdf"、"epub"、"docx"），可传空串让 MuPDF 自动探测
    MuPDFDocument(QString filePath, QString magic = QString());
    ~MuPDFDocument() override;

    bool isValid() const { return m_document != nullptr; }
    QString lastError() const { return m_lastError; }

    QString title() const override;
    int pageCount() const override;
    QImage renderPage(int page, double scale) const override;
    QString pageText(int page) const override;
    QVector<OutlineItem> outline() const override;

private:
    fz_context *m_context = nullptr;
    fz_stream *m_stream = nullptr;
    fz_document *m_document = nullptr;
    QString m_filePath;
    QString m_title;
    QString m_lastError;
};

#endif // MUPDFDOCUMENT_H