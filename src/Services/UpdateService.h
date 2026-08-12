#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Auto-update against a GitHub release (public repo).
// Flow: checkForUpdates() -> latest release tag & exe asset URL;
//       downloadAndInstall() -> download exe, launch installer, quit self.
class UpdateService : public QObject
{
    Q_OBJECT
public:
    explicit UpdateService(QObject *parent = nullptr);

    Q_INVOKABLE void checkForUpdates();          // async check against Gitee releases/latest
    Q_INVOKABLE void downloadAndInstall();       // download the exe asset, run it, then quit
    Q_INVOKABLE QString currentVersion();        // local version "2.1.0"

    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadStateChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY downloadStateChanged)

    bool updateAvailable() const { return m_available; }
    QString latestVersion() const { return m_latest; }
    QString downloadUrl() const { return m_url; }
    bool downloading() const { return m_downloading; }
    int downloadProgress() const { return m_progress; }

signals:
    void updateAvailableChanged();
    void checkFinished(bool available);
    void downloadFinished(bool ok, QString message);
    void downloadStateChanged();

private:
    static bool versionGreater(const QString &remote, const QString &local);
    void parseLatestRelease(const QByteArray &json);

    bool m_available = false;
    QString m_latest;
    QString m_url;              // exe asset download url (public GitHub, direct)
    bool m_downloading = false;
    int m_progress = 0;
    QNetworkAccessManager *m_mgr = nullptr;
};
