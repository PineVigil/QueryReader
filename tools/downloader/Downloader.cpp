#include "Downloader.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void Downloader::download(const QString &url, const QString &tempFile)
{
    // 使用带 UA 的请求，部分 CDN 对默认 UA 会拦截
    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent",
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) QueryReader-Downloader");

    m_reply = m_nam->get(request);

    QFile *file = new QFile(tempFile, this);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit finished(false, QStringLiteral("无法创建临时文件：%1").arg(tempFile));
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }
    file->setParent(m_reply); // 随 reply 一起清理

    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) { emit progress(received, total); });

    connect(m_reply, &QNetworkReply::readyRead, this,
            [this, file]() { file->write(m_replyReadBuffer()); });

    connect(m_reply, &QNetworkReply::finished, this,
            [this, file, tempFile]() {
                const bool ok = (m_reply->error() == QNetworkReply::NoError);
                if (ok) {
                    file->flush();
                    file->close();
                    emit finished(true, tempFile);
                } else {
                    file->remove();
                    emit finished(false, QStringLiteral("下载失败：%1")
                                          .arg(m_reply->errorString()));
                }
                m_reply->deleteLater();
                m_reply = nullptr;
            });
}

// 从当前 m_reply 读取一块数据
QByteArray Downloader::m_replyReadBuffer() const
{
    return m_reply ? m_reply->readAll() : QByteArray();
}

bool Downloader::extract(const QString &zipFile, const QString &destDir, QString *error)
{
    // Win10+ 自带 bsdtar，支持解压 zip；目标目录需存在
    if (!QFile::exists(zipFile)) {
        if (error) {
            *error = QStringLiteral("压缩包不存在：%1").arg(zipFile);
        }
        return false;
    }
    QDir dir(destDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error) {
            *error = QStringLiteral("无法创建安装目录：%1").arg(destDir);
        }
        return false;
    }

    QProcess proc;
    // bsdtar 对 cp932/utf8 文件名有编码处理，windows 下推荐 -o 保留；这里用默认
    proc.start(QStringLiteral("tar"), {QStringLiteral("-xf"), zipFile, QStringLiteral("-C"), destDir});
    if (!proc.waitForFinished(-1)) {
        if (error) {
            *error = QStringLiteral("解压进程启动失败");
        }
        return false;
    }
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        if (error) {
            *error = QStringLiteral("解压失败：%1").arg(QString::fromLocal8Bit(proc.readAllStandardOutput())
                                                            .simplified());
        }
        return false;
    }
    return true;
}

bool Downloader::launch(const QString &exePath, QString *error)
{
    if (!QFile::exists(exePath)) {
        if (error) {
            *error = QStringLiteral("程序不存在：%1").arg(exePath);
        }
        return false;
    }
    if (!QProcess::startDetached(exePath, QStringList())) {
        if (error) {
            *error = QStringLiteral("启动程序失败：%1").arg(exePath);
        }
        return false;
    }
    return true;
}