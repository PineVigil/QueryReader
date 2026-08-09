#include "core/MuPDFDocument.h"

#include <QFileInfo>
#include <QImage>

#include <string>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

MuPDFDocument::MuPDFDocument(QString filePath, QString magic)
    : m_filePath(std::move(filePath))
{
    m_context = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
    if (!m_context) {
        return;
    }
    fz_register_document_handlers(m_context);
    fz_try(m_context)
    {
        // 用 wide 路径（fz_open_file_w）避免中文等非 ANSI 路径失败
        const std::wstring wide = m_filePath.toStdWString();
        fz_stream *stream = fz_open_file_w(m_context, wide.c_str());
        if (!stream) {
            fz_throw(m_context, FZ_ERROR_GENERIC, "cannot open file: %S", wide.c_str());
        }
        const QByteArray magicBytes = magic.toLatin1();
        m_document = fz_open_document_with_stream(
            m_context, magic.isEmpty() ? nullptr : magicBytes.constData(), stream);
        fz_drop_stream(m_context, stream); // 调用方保留所有权，需自行释放
        m_title = QFileInfo(m_filePath).completeBaseName();
    }
    fz_catch(m_context)
    {
        m_document = nullptr;
        const char *msg = fz_caught_message(m_context);
        m_lastError = QString::fromUtf8(msg ? msg : "unknown error");
    }
}

MuPDFDocument::~MuPDFDocument()
{
    if (m_document) {
        fz_drop_document(m_context, m_document);
    }
    if (m_context) {
        fz_drop_context(m_context);
    }
}

QString MuPDFDocument::title() const
{
    return m_title;
}

int MuPDFDocument::pageCount() const
{
    if (!m_document) {
        return 0;
    }
    int n = 0;
    fz_try(m_context)
    {
        n = fz_count_pages(m_context, m_document);
    }
    fz_catch(m_context)
    {
        n = 0;
    }
    return n;
}

QImage MuPDFDocument::renderPage(int page, double scale) const
{
    if (!m_document || page < 0 || page >= pageCount()) {
        return QImage();
    }

    QImage result;
    fz_try(m_context)
    {
        // scale 为 UI 缩放系数 (1.0=100%)，按 150 DPI 渲染保证清晰
        const float zoom = static_cast<float>(scale) * 150.0f / 72.0f;
        const fz_matrix ctm = fz_scale(zoom, zoom);

        fz_pixmap *pix = fz_new_pixmap_from_page_number(
            m_context, m_document, page, ctm,
            fz_device_rgb(m_context), 0);

        if (!pix) {
            fz_throw(m_context, FZ_ERROR_GENERIC, "failed to render page %d", page);
        }

        const int w = pix->w;
        const int h = pix->h;
        const int n = pix->n;

        QImage img(w, h, QImage::Format_RGB888);
        for (int y = 0; y < h; ++y) {
            const unsigned char *src = pix->samples + static_cast<size_t>(y) * pix->stride;
            unsigned char *dst = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                dst[x * 3] = src[x * n];
                dst[x * 3 + 1] = src[x * n + 1];
                dst[x * 3 + 2] = src[x * n + 2];
            }
        }

        fz_drop_pixmap(m_context, pix);
        result = std::move(img);
    }
    fz_catch(m_context)
    {
        return QImage();
    }
    return result;
}

QString MuPDFDocument::pageText(int page) const
{
    if (!m_document || page < 0 || page >= pageCount()) {
        return QString();
    }

    QString text;
    fz_try(m_context)
    {
        fz_stext_options opts;
        fz_init_stext_options(m_context, &opts);
        // 让 stext 保留必要空白，便于搜索/复制
        opts.flags |= FZ_STEXT_PRESERVE_WHITESPACE;
        fz_stext_page *stext = fz_new_stext_page_from_page_number(
            m_context, m_document, page, &opts);
        if (!stext) {
            fz_throw(m_context, FZ_ERROR_GENERIC, "failed to extract text page %d", page);
        }
        fz_buffer *buf = fz_new_buffer(m_context, 1024);
        fz_output *out = fz_new_output_with_buffer(m_context, buf);
        fz_print_stext_page_as_text(m_context, out, stext);
        fz_drop_output(m_context, out);
        text = QString::fromUtf8(fz_string_from_buffer(m_context, buf));
        fz_drop_buffer(m_context, buf);
        fz_drop_stext_page(m_context, stext);
    }
    fz_catch(m_context)
    {
        return QString();
    }
    return text;
}

namespace {

// 递归展开 fz_outline 树为平铺列表
void flattenOutline(QVector<OutlineItem> &items, const fz_outline *node, int level, int &depth)
{
    for (; node; node = node->next) {
        OutlineItem oi;
        if (node->title) {
            oi.title = QString::fromUtf8(node->title);
        }
        oi.level = level;
        if (node->page.page >= 0) {
            oi.page = node->page.page;
        }
        items.append(oi);
        if (node->down) {
            flattenOutline(items, node->down, level + 1, depth);
        }
    }
}

} // namespace

QVector<OutlineItem> MuPDFDocument::outline() const
{
    QVector<OutlineItem> items;
    if (!m_document) {
        return items;
    }

    fz_try(m_context)
    {
        fz_outline_iterator *iter = fz_new_outline_iterator(m_context, m_document);
        if (!iter) {
            fz_throw(m_context, FZ_ERROR_GENERIC, "no outline");
        }
        fz_outline *root = fz_load_outline_from_iterator(m_context, iter);
        fz_drop_outline_iterator(m_context, iter);
        if (!root) {
            fz_throw(m_context, FZ_ERROR_GENERIC, "no outline items");
        }

        int depth = 0;
        flattenOutline(items, root, 0, depth);
        fz_drop_outline(m_context, root);
    }
    fz_catch(m_context)
    {
        items.clear();
    }
    return items;
}