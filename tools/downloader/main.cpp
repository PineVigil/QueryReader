#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "Downloader.h"

static bool createDesktopShortcut(const QString &exePath, QString *error)
{
    const QString desktop =
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString lnkPath = desktop + QStringLiteral("\\QueryReader.lnk");

    // PowerShell: 用 WScript.Shell COM 对象创建 .lnk
    QProcess proc;
    proc.start(QStringLiteral("powershell"),
               {QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                QStringLiteral(
                    "$s=(New-Object -COM WScript.Shell).CreateShortcut('%1');"
                    "$s.TargetPath='%2';$s.WorkingDirectory='%3';"
                    "$s.Description='QueryReader 轻量阅读器';$s.Save()")
                    .arg(lnkPath, QDir::toNativeSeparators(exePath),
                         QDir::toNativeSeparators(QFileInfo(exePath).path()))});
    if (!proc.waitForFinished(5000) || proc.exitCode() != 0) {
        if (error) {
            *error = QStringLiteral("创建快捷方式失败");
        }
        return false;
    }
    return true;
}

static const QString kDownloadUrl =
    QStringLiteral("https://github.com/PineVigil/QueryReader/"
                   "releases/latest/download/QueryReader-win64.zip");

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

    // 安装目录
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

    // 进度条
    auto *prog = new QProgressBar(&dlg);
    prog->setRange(0, 100);
    prog->setValue(0);
    prog->setTextVisible(true);
    layout->addWidget(prog);

    // 状态
    auto *statusLabel = new QLabel(QStringLiteral("等待开始…"), &dlg);
    layout->addWidget(statusLabel);

    // 选项
    auto *launchCheck = new QCheckBox(QStringLiteral("安装完成后启动 QueryReader"), &dlg);
    launchCheck->setChecked(true);
    layout->addWidget(launchCheck);

    auto *shortcutCheck = new QCheckBox(QStringLiteral("创建桌面快捷方式"), &dlg);
    shortcutCheck->setChecked(true);
    layout->addWidget(shortcutCheck);

    // 按钮
    auto *installBtn = new QPushButton(QStringLiteral("开始安装"), &dlg);
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), &dlg);
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    btnRow->addWidget(installBtn);
    layout->addLayout(btnRow);

    Downloader downloader;

    // 关闭 = 中止下载 + 删临时文件 + 退出
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, [&downloader, &dlg]() {
        downloader.cancel();
        dlg.reject();
    });

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

    // 下载完成
    QObject::connect(&downloader, &Downloader::finished, &dlg,
                     [&dlg, &downloader, dirEdit, prog, statusLabel, installBtn, closeBtn,
                      launchCheck, shortcutCheck](bool ok, const QString &message) {
                         installBtn->setEnabled(true);
                         closeBtn->setEnabled(true);

                         if (!ok) {
                             statusLabel->setText(message);
                             QMessageBox::warning(&dlg, QStringLiteral("下载失败"), message);
                             return;
                         }

                         const QString zipFile = message;
                         const QString destDir = dirEdit->text();

                         statusLabel->setText(QStringLiteral("正在解压…"));
                         QString error;
                         if (!downloader.extract(zipFile, destDir, &error)) {
                             QMessageBox::warning(&dlg, QStringLiteral("解压失败"), error);
                             statusLabel->setText(error);
                             return;
                         }
                         QFile::remove(zipFile);

                         const QString exePath =
                             destDir + QStringLiteral("/QueryReader.exe");

                         // 桌面快捷方式
                         if (shortcutCheck->isChecked()) {
                             QString scErr;
                             createDesktopShortcut(exePath, &scErr);
                         }

                         // 启动
                         if (launchCheck->isChecked()) {
                             QString launchErr;
                             if (!downloader.launch(exePath, &launchErr)) {
                                 QMessageBox::information(
                                     &dlg, QStringLiteral("安装完成"),
                                     QStringLiteral("安装完成：%1\n（未能自动启动：%2）")
                                         .arg(destDir, launchErr));
                             } else {
                                 statusLabel->setText(
                                     QStringLiteral("安装完成：%1").arg(destDir));
                                 QMessageBox::information(
                                     &dlg, QStringLiteral("安装完成"),
                                     QStringLiteral("QueryReader 已安装并启动。"));
                             }
                         } else {
                             statusLabel->setText(
                                 QStringLiteral("安装完成：%1").arg(destDir));
                             QMessageBox::information(
                                 &dlg, QStringLiteral("安装完成"),
                                 QStringLiteral("安装完成：%1\nQueryReader 未启动。")
                                     .arg(destDir));
                         }

                         prog->setValue(prog->maximum());
                     });

    // 开始安装
    QObject::connect(installBtn, &QPushButton::clicked, &dlg,
                     [&dlg, dirEdit, prog, statusLabel, installBtn, closeBtn, &downloader]() {
                         const QString destDir = dirEdit->text().trimmed();
                         if (destDir.isEmpty()) {
                             QMessageBox::warning(&dlg, QStringLiteral("提示"),
                                                  QStringLiteral("请选择安装目录。"));
                             return;
                         }
                         installBtn->setEnabled(false);
                         closeBtn->setEnabled(false);
                         prog->setValue(0);
                         statusLabel->setText(QStringLiteral("准备下载…"));

                         const QString tempFile =
                             QDir::temp().filePath(QStringLiteral("queryreader_download.zip"));
                         QFile::remove(tempFile);
                         downloader.download(kDownloadUrl, tempFile);
                     });

    dlg.exec();
    return 0;
}
