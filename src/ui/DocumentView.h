#ifndef DOCUMENTVIEW_H
#define DOCUMENTVIEW_H

#include <QLabel>
#include <QScrollArea>
#include <QSharedPointer>

#include "core/Document.h"

// 单页显示视图：展示当前页图片，支持翻页、缩放、键盘操作。
class DocumentView : public QScrollArea
{
    Q_OBJECT

public:
    explicit DocumentView(QWidget *parent = nullptr);

    void setDocument(const QSharedPointer<Document> &doc);
    void goToPage(int page);
    void nextPage();
    void prevPage();

    void zoomIn();
    void zoomOut();
    void resetZoom();

    int currentPage() const { return m_currentPage; }
    int pageCount() const { return m_doc ? m_doc->pageCount() : 0; }
    double zoom() const { return m_zoom; }
    QSharedPointer<Document> document() const { return m_doc; }

signals:
    void pageChanged(int page, int total);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void renderCurrent();
    double clampZoom(double z) const;

    QSharedPointer<Document> m_doc;
    int m_currentPage = 0;
    double m_zoom = 1.0;
    QLabel *m_pageLabel;
};

#endif // DOCUMENTVIEW_H