#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);

    void download(const QString &url, const QString &tempFile);
    void cancel();
    bool extract(const QString &zipFile, const QString &destDir, QString *error);
    bool launch(const QString &exePath, QString *error);

signals:
    void progress(qint64 received, qint64 total);
    void finished(bool ok, const QString &message);

private:
    QByteArray m_replyReadBuffer() const;

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    bool m_aborting = false;
    QString m_tempFile;
};

#endif // DOWNLOADER_H
