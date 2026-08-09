#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class DocumentManager;
class DocumentView;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void openPath(const QString &filePath);

private:
    void setupUi();
    void openDocument();
    void populateOutline();
    void onOutlineClicked(QTreeWidgetItem *item, int column);
    void findNext();
    void goToPage();
    void updatePageLabel(int page, int total);

    DocumentManager *m_manager = nullptr;
    DocumentView *m_view = nullptr;
    QTreeWidget *m_outlineTree = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QLineEdit *m_pageEdit = nullptr;
};

#endif // MAINWINDOW_H