#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// 负责从 URL 下载 zip 包、解压到目标目录、并运行其中的主程序。
class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);

    // 下载 url 的 zip 到 tempFile，完成后信号通知
    void download(const QString &url, const QString &tempFile);
    // 将 zipFile 解压到 destDir（调用系统 tar）
    bool extract(const QString &zipFile, const QString &destDir, QString *error);
    // 启动 destDir 下的可执行程序（非阻塞）
    bool launch(const QString &exePath, QString *error);

signals:
    void progress(qint64 received, qint64 total);
    void finished(bool ok, const QString &message);

private:
    QByteArray m_replyReadBuffer() const;

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
};

#endif // DOWNLOADER_H