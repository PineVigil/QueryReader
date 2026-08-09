#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QString>
#include <QToolBar>
#include <QTreeWidget>

#include "core/DocumentManager.h"
#include "parsers/MarkdownParser.h"
#include "parsers/MuPDFParser.h"
#include "parsers/TxtParser.h"
#include "ui/DocumentView.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_manager = new DocumentManager();
    m_manager->registerParser(new TxtParser());
    m_manager->registerParser(new MarkdownParser());
    m_manager->registerParser(new MuPDFParser());

    setupUi();
}

void MainWindow::setupUi()
{
    setWindowTitle(tr("QueryReader - 轻量阅读器"));
    resize(1100, 800);

    // 文件菜单
    QAction *openAction = new QAction(tr("打开(&O)"), this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);

    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    fileMenu->addAction(openAction);

    // 工具栏
    QToolBar *toolbar = addToolBar(tr("工具栏"));
    toolbar->setMovable(false);

    QAction *openBtn = toolbar->addAction(tr("打开"));
    connect(openBtn, &QAction::triggered, this, &MainWindow::openDocument);

    toolbar->addSeparator();

    QAction *zoomInAction = toolbar->addAction(tr("放大"));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);

    toolbar->addSeparator();

    // 搜索框
    m_searchEdit = new QLineEdit(toolbar);
    m_searchEdit->setPlaceholderText(tr("搜索…"));
    m_searchEdit->setMaximumWidth(200);
    m_searchEdit->setClearButtonEnabled(true);
    toolbar->addWidget(m_searchEdit);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);

    // 中央视图
    m_view = new DocumentView(this);
    setCentralWidget(m_view);
    connect(m_view, &DocumentView::pageChanged, this, &MainWindow::updatePageLabel);

    // 缩放动作（view 已创建后连接）
    connect(zoomInAction, &QAction::triggered, m_view, &DocumentView::zoomIn);

    QAction *zOut = new QAction(tr("缩小(&-)"), this);
    zOut->setShortcut(QKeySequence::ZoomOut);
    connect(zOut, &QAction::triggered, m_view, &DocumentView::zoomOut);
    toolbar->addAction(zOut);

    QAction *zReset = new QAction(tr("原始大小"), this);
    connect(zReset, &QAction::triggered, m_view, &DocumentView::resetZoom);
    toolbar->addAction(zReset);

    toolbar->addSeparator();

    // 页码跳转
    QLabel *pageLabel = new QLabel(tr("页码:"), toolbar);
    toolbar->addWidget(pageLabel);
    m_pageEdit = new QLineEdit(toolbar);
    m_pageEdit->setMaximumWidth(60);
    toolbar->addWidget(m_pageEdit);
    connect(m_pageEdit, &QLineEdit::returnPressed, this, &MainWindow::goToPage);

    // 目录侧边栏
    QDockWidget *dock = new QDockWidget(tr("目录"), this);
    dock->setObjectName(QStringLiteral("outlineDock"));
    m_outlineTree = new QTreeWidget(dock);
    m_outlineTree->setHeaderHidden(true);
    dock->setWidget(m_outlineTree);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    connect(m_outlineTree, &QTreeWidget::itemClicked, this, &MainWindow::onOutlineClicked);

    statusBar()->showMessage(tr("就绪"));
}

void MainWindow::openPath(const QString &filePath)
{
    auto doc = m_manager->openFile(filePath);
    if (!doc) {
        statusBar()->showMessage(tr("无法打开该文档：%1").arg(filePath), 5000);
        return;
    }

    m_view->setDocument(doc);
    setWindowTitle(QStringLiteral("%1 - QueryReader").arg(doc->title()));
    populateOutline();
    statusBar()->showMessage(tr("已打开：%1（%2 页）").arg(doc->title()).arg(doc->pageCount()));
}

void MainWindow::openDocument()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("打开文档"), QString(),
        m_manager->supportedFilter());

    if (filePath.isEmpty()) {
        return;
    }

    openPath(filePath);
}

void MainWindow::populateOutline()
{
    m_outlineTree->clear();
    if (!m_view || !m_view->document()) {
        return;
    }
    const QVector<OutlineItem> outline = m_view->document()->outline();
    if (outline.isEmpty()) {
        m_outlineTree->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("（无目录）")));
        return;
    }

    QTreeWidgetItem *currentLevelParent = nullptr;
    QList<QTreeWidgetItem *> levelParents; // 各层级的父节点
    for (const OutlineItem &oi : outline) {
        auto *item = new QTreeWidgetItem(QStringList() << oi.title);
        item->setData(0, Qt::UserRole, oi.page);

        // 按层级放到正确的父节点下
        while (levelParents.size() > oi.level) {
            levelParents.removeLast();
        }
        if (levelParents.isEmpty()) {
            m_outlineTree->addTopLevelItem(item);
        } else {
            levelParents.last()->addChild(item);
        }
        levelParents.append(item);
    }
    m_outlineTree->expandAll();
}

void MainWindow::onOutlineClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !m_view) {
        return;
    }
    const QVariant page = item->data(0, Qt::UserRole);
    if (page.isValid()) {
        m_view->goToPage(page.toInt());
    }
}

void MainWindow::findNext()
{
    if (!m_view || !m_view->document()) {
        return;
    }
    const QString needle = m_searchEdit->text().trimmed();
    if (needle.isEmpty()) {
        return;
    }

    const int total = m_view->document()->pageCount();
    const int start = m_view->currentPage();
    bool found = false;

    for (int step = 0; step < total; ++step) {
        const int page = (start + step) % total;
        const QString text = m_view->document()->pageText(page);
        if (text.contains(needle, Qt::CaseInsensitive)) {
            m_view->goToPage(page);
            statusBar()->showMessage(tr("找到：第 %1 页").arg(page + 1), 3000);
            found = true;
            break;
        }
    }
    if (!found) {
        statusBar()->showMessage(tr("未找到：%1").arg(needle), 3000);
    }
}

void MainWindow::goToPage()
{
    if (!m_view || !m_pageEdit) {
        return;
    }
    bool ok = false;
    const int page = m_pageEdit->text().toInt(&ok);
    if (ok) {
        m_view->goToPage(page - 1); // UI 页码为 1 基，内部为 0 基
    }
}

void MainWindow::updatePageLabel(int page, int total)
{
    if (total == 0) {
        m_pageEdit->clear();
        statusBar()->showMessage(tr("就绪"));
        return;
    }
    m_pageEdit->setText(QString::number(page));
    const int percent = qRound(m_view->zoom() * 100);
    statusBar()->showMessage(tr("第 %1 / %2 页　　缩放 %3%").arg(page).arg(total).arg(percent));
}