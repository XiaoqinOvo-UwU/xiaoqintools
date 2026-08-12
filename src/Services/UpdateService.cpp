#include "UpdateService.h"
#include "ConfigService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QTimer>
#include <QRegularExpression>

UpdateService::UpdateService(QObject *parent)
    : QObject(parent)
{
}

QString UpdateService::currentVersion()
{
    return QCoreApplication::applicationVersion();
}

QString UpdateService::authUrl(const QString &base) const
{
    QString token = ConfigService::instance().giteeToken().trimmed();
    if (token.isEmpty()) return base;
    if (base.contains('?'))
        return base + "&access_token=" + token;
    return base + "?access_token=" + token;
}

// compare "v2.1.1" style tags; true if remote > local
bool UpdateService::versionGreater(const QString &remote, const QString &local)
{
    auto nums = [](const QString &s) {
        QStringList out;
        for (const QString &seg : s.split(QRegularExpression("[^0-9]+"), Qt::SkipEmptyParts))
            out << seg;
        return out;
    };
    QStringList r = nums(remote), l = nums(local);
    int n = qMax(r.size(), l.size());
    for (int i = 0; i < n; i++) {
        int rv = i < r.size() ? r[i].toInt() : 0;
        int lv = i < l.size() ? l[i].toInt() : 0;
        if (rv != lv) return rv > lv;
    }
    return false; // equal
}

void UpdateService::parseLatestRelease(const QByteArray &data)
{
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        emit checkFinished(false);
        return;
    }
    QJsonObject o = doc.object();
    QString tag = o.value("tag_name").toString();
    if (tag.isEmpty()) tag = o.value("name").toString();

    // find an exe asset
    QString assetUrl;
    QString assetId;
    QJsonArray assets = o.value("assets").toArray();
    for (const QJsonValue &v : assets) {
        QJsonObject a = v.toObject();
        QString name = a.value("name").toString().toLower();
        if (!name.endsWith(".exe")) continue;
        assetUrl = a.value("browser_download_url").toString();
        assetId = QString::number(a.value("id").toDouble());
        break;
    }
    if (assetId.isEmpty() || o.value("id").isUndefined()) {
        // older gitee releases may not expose assets in this response; fallback:
        // store release id only if present
        assetId.clear();
    }
    m_releaseId = QString::number(o.value("id").toDouble());
    m_assetId = assetId;

    QString local = currentVersion();
    if (tag.isEmpty()) {
        emit checkFinished(false);
        return;
    }
    if (!versionGreater(tag, local)) {
        emit checkFinished(false); // already up to date
        return;
    }
    m_latest = tag;
    // build the API download url (works for private repos with token)
    if (!m_assetId.isEmpty() && !m_releaseId.isEmpty()) {
        QString repo = ConfigService::instance().giteeRepo().trimmed();
        if (repo.isEmpty()) repo = "xiao-qin-uwu/xiaoqintools";
        m_url = authUrl("https://gitee.com/api/v5/repos/" + repo
                        + "/releases/" + m_releaseId
                        + "/attach_files/" + m_assetId + "/download");
    } else {
        m_url = assetUrl; // public repo fallback
    }
    m_available = true;
    emit updateAvailableChanged();
    emit checkFinished(true);
}

void UpdateService::checkForUpdates()
{
    QString repo = ConfigService::instance().giteeRepo().trimmed();
    if (repo.isEmpty()) repo = "xiao-qin-uwu/xiaoqintools";
    QString api = authUrl("https://gitee.com/api/v5/repos/" + repo + "/releases/latest");

    if (!m_mgr) m_mgr = new QNetworkAccessManager(this);
    QNetworkRequest req;
    req.setUrl(QUrl(api));
    req.setRawHeader("User-Agent", "XiaoQinTools");
    QNetworkReply *reply = m_mgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit checkFinished(false);
            return;
        }
        parseLatestRelease(reply->readAll());
    });
}

void UpdateService::downloadAndInstall()
{
    if (m_downloading || m_url.isEmpty()) return;
    m_downloading = true;
    m_progress = 0;
    emit downloadStateChanged();

    // target dir: %TEMP%/XiaoQinToolsUpdate
    QString dir = QDir::temp().filePath("XiaoQinToolsUpdate");
    QDir().mkpath(dir);
    QString fileName = QUrl(m_url).fileName();
    if (fileName.isEmpty()) fileName = "update.exe";
    QString dest = dir + "/" + fileName;

    if (!m_mgr) m_mgr = new QNetworkAccessManager(this);
    QNetworkRequest req;
    req.setUrl(QUrl(m_url)); // already includes access_token via authUrl()
    req.setRawHeader("User-Agent", "XiaoQinTools");
    QNetworkReply *reply = m_mgr->get(req);
    QFile *out = new QFile(dest);
    if (!out->open(QIODevice::WriteOnly)) {
        delete out;
        m_downloading = false;
        emit downloadFinished(false, "无法创建下载文件");
        return;
    }
    connect(reply, &QNetworkReply::readyRead, this, [reply, out]() {
        out->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 got, qint64 total) {
        m_progress = total > 0 ? (int)(got * 100 / total) : 0;
        emit downloadStateChanged();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, out, dest]() {
        out->flush();
        out->close();
        delete out;
        reply->deleteLater();
        m_downloading = false;
        if (reply->error() != QNetworkReply::NoError) {
            emit downloadStateChanged();
            emit downloadFinished(false, "下载失败：" + reply->errorString());
            return;
        }
        emit downloadStateChanged();

        // launch the installer, then exit ourselves so the new version can replace files
        QProcess::startDetached(dest, QStringList());
        QTimer::singleShot(1500, qApp, &QCoreApplication::quit);
        emit downloadFinished(true, "安装包已下载，安装程序即将启动~");
    });
}
