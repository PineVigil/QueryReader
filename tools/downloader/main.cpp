#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "Downloader.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QDialog dlg;
    dlg.setWindowTitle(QStringLiteral("QueryReader 安装器"));
    dlg.setMinimumWidth(480);

    auto *layout = new QVBoxLayout(&dlg);

    auto *desc = new QLabel(
        QStringLiteral("本向导将从网络下载 QueryReader 阅读器并安装到你的电脑。\n"
                       "请选择安装位置，然后点击「开始安装」。"),
        &dlg);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto *dirRow = new QHBoxLayout;
    auto *dirLabel = new QLabel(QStringLiteral("安装目录:"), &dlg);
    auto *dirEdit = new QLineEdit(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
            + QStringLiteral("/QueryReader"),
        &dlg);
    auto *dirBtn = new QPushButton(QStringLiteral("浏览…"), &dlg);
    dirRow->addWidget(dirLabel);
    dirRow->addWidget(dirEdit, 1);
    dirRow->addWidget(dirBtn);
    layout->addLayout(dirRow);

    QObject::connect(dirBtn, &QPushButton::clicked, &dlg, [&dlg, dirEdit]() {
        const QString dir = QFileDialog::getExistingDirectory(
            &dlg, QStringLiteral("选择安装目录"), dirEdit->text());
        if (!dir.isEmpty()) {
            dirEdit->setText(dir + QStringLiteral("/QueryReader"));
        }
    });

    // 下载地址（固定指向 GitHub Release 的 zip 附件，用户无需修改即可直接安装）
    auto *urlRow = new QHBoxLayout;
    auto *urlLabel = new QLabel(QStringLiteral("下载源:"), &dlg);
    auto *urlEdit = new QLineEdit(
        QStringLiteral("https://github.com/PineVigil/QueryReader/releases/latest/download/QueryReader-win64.zip"),
        &dlg);
    urlEdit->setPlaceholderText(QStringLiteral("https://github.com/PineVigil/QueryReader/releases/latest/download/QueryReader-win64.zip"));
    urlRow->addWidget(urlLabel);
    urlRow->addWidget(urlEdit, 1);
    layout->addLayout(urlRow);

    auto *prog = new QProgressBar(&dlg);
    prog->setRange(0, 100);
    prog->setValue(0);
    prog->setTextVisible(true);
    layout->addWidget(prog);

    auto *statusLabel = new QLabel(QStringLiteral("等待开始…"), &dlg);
    layout->addWidget(statusLabel);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(buttons);
    auto *installBtn = new QPushButton(QStringLiteral("开始安装"), &dlg);
    buttons->addButton(installBtn, QDialogButtonBox::ActionRole);

    Downloader downloader;

    // 下载进度
    QObject::connect(&downloader, &Downloader::progress, &dlg,
                     [prog, statusLabel](qint64 received, qint64 total) {
                         if (total > 0) {
                             prog->setMaximum(static_cast<int>(total));
                             prog->setValue(static_cast<int>(received));
                             statusLabel->setText(
                                 QStringLiteral("正在下载… %1 / %2 KB")
                                     .arg(received / 1024)
                                     .arg(total / 1024));
                         } else {
                             statusLabel->setText(
                                 QStringLiteral("正在下载… %1 KB").arg(received / 1024));
                         }
                     });

    // 下载完成：解压 → 启动
    QObject::connect(&downloader, &Downloader::finished, &dlg,
                     [&dlg, &downloader, dirEdit, prog, statusLabel, installBtn, dirBtn](
                         bool ok, const QString &message) {
                         installBtn->setEnabled(true);
                         if (!ok) {
                             statusLabel->setText(message);
                             QMessageBox::warning(&dlg, QStringLiteral("下载失败"), message);
                             return;
                         }
                         const QString zipFile = message; // finished(true) 时 message 是临时 zip 路径
                         const QString destDir = dirEdit->text();

                         statusLabel->setText(QStringLiteral("正在解压…"));
                         QString error;
                         if (!downloader.extract(zipFile, destDir, &error)) {
                             QMessageBox::warning(&dlg, QStringLiteral("解压失败"), error);
                             statusLabel->setText(error);
                             return;
                         }
                         QFile::remove(zipFile); // 清理临时包

                         const QString exePath = destDir + QStringLiteral("/QueryReader.exe");
                         if (!downloader.launch(exePath, &error)) {
                             QMessageBox::information(
                                 &dlg, QStringLiteral("安装完成"),
                                 QStringLiteral("安装完成：%1\n（未能自动启动：%2）")
                                     .arg(destDir)
                                     .arg(error));
                         } else {
                             statusLabel->setText(
                                 QStringLiteral("安装完成：%1").arg(destDir));
                             QMessageBox::information(&dlg, QStringLiteral("安装完成"),
                                                      QStringLiteral("QueryReader 已安装并启动。"));
                         }
                         prog->setValue(prog->maximum());
                     });

    // 开始安装
    QObject::connect(installBtn, &QPushButton::clicked, &dlg,
                     [&dlg, urlEdit, dirEdit, prog, statusLabel, installBtn, &downloader]() {
                         const QString url = urlEdit->text().trimmed();
                         const QString destDir = dirEdit->text().trimmed();
                         if (url.isEmpty()) {
                             QMessageBox::warning(&dlg, QStringLiteral("提示"),
                                                  QStringLiteral("请填写下载源地址。"));
                             return;
                         }
                         installBtn->setEnabled(false);
                         prog->setValue(0);
                         statusLabel->setText(QStringLiteral("准备下载…"));

                         const QString tempFile =
                             QDir::temp().filePath(QStringLiteral("queryreader_download.zip"));
                         QFile::remove(tempFile);
                         downloader.download(url, tempFile);
                     });

    dlg.exec();
    return 0;
}