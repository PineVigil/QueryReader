#include "ui/DocumentView.h"

#include <QKeyEvent>
#include <QScrollBar>
#include <QWheelEvent>

DocumentView::DocumentView(QWidget *parent)
    : QScrollArea(parent)
    , m_pageLabel(new QLabel(this))
{
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setBackgroundRole(QPalette::Base);
    setWidget(m_pageLabel);
    setWidgetResizable(true);
    setAlignment(Qt::AlignCenter);
    setBackgroundRole(QPalette::Dark);
}

void DocumentView::setDocument(const QSharedPointer<Document> &doc)
{
    m_doc = doc;
    m_currentPage = 0;
    m_zoom = 1.0;
    if (m_doc) {
        renderCurrent();
        emit pageChanged(m_currentPage + 1, m_doc->pageCount());
    } else {
        m_pageLabel->setPixmap(QPixmap());
        m_pageLabel->setText(tr("暂无文档"));
        emit pageChanged(0, 0);
    }
}

void DocumentView::goToPage(int page)
{
    if (!m_doc || page < 0 || page >= m_doc->pageCount()) {
        return;
    }
    // 重置滚动位置，避免残留旧页滚动
    verticalScrollBar()->setValue(0);
    horizontalScrollBar()->setValue(0);
    m_currentPage = page;
    renderCurrent();
    emit pageChanged(m_currentPage + 1, m_doc->pageCount());
}

void DocumentView::nextPage()
{
    if (m_doc) {
        goToPage(m_currentPage + 1);
    }
}

void DocumentView::prevPage()
{
    if (m_doc) {
        goToPage(m_currentPage - 1);
    }
}

void DocumentView::zoomIn()
{
    m_zoom = clampZoom(m_zoom * 1.25);
    renderCurrent();
    emit pageChanged(m_currentPage + 1, m_doc ? m_doc->pageCount() : 0);
}

void DocumentView::zoomOut()
{
    m_zoom = clampZoom(m_zoom / 1.25);
    renderCurrent();
    emit pageChanged(m_currentPage + 1, m_doc ? m_doc->pageCount() : 0);
}

void DocumentView::resetZoom()
{
    m_zoom = 1.0;
    renderCurrent();
    emit pageChanged(m_currentPage + 1, m_doc ? m_doc->pageCount() : 0);
}

double DocumentView::clampZoom(double z) const
{
    return qBound(0.1, z, 4.0);
}

void DocumentView::renderCurrent()
{
    if (!m_doc) {
        return;
    }
    QImage image = m_doc->renderPage(m_currentPage, m_zoom);
    m_pageLabel->setPixmap(QPixmap::fromImage(image));
}

void DocumentView::wheelEvent(QWheelEvent *event)
{
    // Ctrl + 滚轮缩放
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta > 0) {
            zoomIn();
        } else if (delta < 0) {
            zoomOut();
        }
        event->accept();
        return;
    }
    QScrollArea::wheelEvent(event);
}

void DocumentView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_PageDown:
    case Qt::Key_Right:
    case Qt::Key_Space:
    case Qt::Key_Down:
        nextPage();
        event->accept();
        return;
    case Qt::Key_PageUp:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Backspace:
        prevPage();
        event->accept();
        return;
    case Qt::Key_Home:
        goToPage(0);
        event->accept();
        return;
    case Qt::Key_End:
        if (m_doc) {
            goToPage(m_doc->pageCount() - 1);
        }
        event->accept();
        return;
    default:
        break;
    }
    QScrollArea::keyPressEvent(event);
}

void DocumentView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击在 100% 与适合宽度之间切换
    if (m_zoom == 1.0) {
        resetZoom();
        if (m_doc) {
            // 尽量适配视口宽度
            QImage first = m_doc->renderPage(m_currentPage, 1.0);
            if (!first.isNull() && viewport()->width() > 0) {
                const double fit = static_cast<double>(viewport()->width())
                                  / static_cast<double>(first.width());
                m_zoom = clampZoom(fit);
                renderCurrent();
            }
        }
    } else {
        resetZoom();
    }
    event->accept();
}