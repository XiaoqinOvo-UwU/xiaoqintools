#include "UpdateService.h"
#include "ConfigService.h"
#include "ProxyService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QTimer>
#include <QRegularExpression>
#include <QNetworkProxy>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QtConcurrent>

UpdateService::UpdateService(QObject *parent)
    : QObject(parent)
{
}

QString UpdateService::currentVersion()
{
    return QCoreApplication::applicationVersion();
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
        m_lastError = "检查失败：无法解析服务器返回（可能是网络/代理问题）";
        emit checkFinished(false);
        return;
    }
    QJsonObject o = doc.object();
    QString tag = o.value("tag_name").toString();
    if (tag.isEmpty()) tag = o.value("name").toString();

    // find a release asset: prefer .zip (green edition, Defender-friendly), fallback .exe
    QString assetUrl;
    QJsonArray assets = o.value("assets").toArray();
    for (const QJsonValue &v : assets) {
        QJsonObject a = v.toObject();
        QString name = a.value("name").toString().toLower();
        if (name.endsWith(".zip")) {
            assetUrl = a.value("browser_download_url").toString();
            break;
        }
    }
    if (assetUrl.isEmpty()) {
        for (const QJsonValue &v : assets) {
            QJsonObject a = v.toObject();
            QString name = a.value("name").toString().toLower();
            if (name.endsWith(".exe")) {
                assetUrl = a.value("browser_download_url").toString();
                break;
            }
        }
    }

    QString local = currentVersion();
    if (tag.isEmpty()) {
        m_lastError = "检查失败：服务器返回的版本信息为空";
        emit checkFinished(false);
        return;
    }
    if (!versionGreater(tag, local)) {
        m_lastError.clear(); // truly up to date
        emit checkFinished(false); // already up to date
        return;
    }
    m_latest = tag;
    m_url = assetUrl; // public repo: direct download works
    m_available = true;
    m_lastError.clear();
    emit updateAvailableChanged();
    emit checkFinished(true);
}

void UpdateService::checkForUpdates()
{
    // GitHub releases API (public repo, no token needed for read).
    QString api = "https://api.github.com/repos/XiaoqinOvo-UwU/xiaoqintools/releases/latest";

    m_lastError.clear();
    m_available = false;
    emit updateAvailableChanged();

    if (!m_mgr) m_mgr = new QNetworkAccessManager(this);
    // GitHub needs a proxy in CN. Prefer the system proxy (works for Clash
    // TUN/mixed mode and v2rayN), fall back to the known ports.
    m_mgr->setProxy(QNetworkProxy::applicationProxy()); // system proxy if any
    ProxyService probe;
    if (m_mgr->proxy().type() == QNetworkProxy::NoProxy) {
        if (probe.isClashPortOpen())
            m_mgr->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 7897));
        else if (probe.isV2rayPortOpen())
            m_mgr->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 10808));
        else
            m_mgr->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    }

    QNetworkRequest req;
    req.setUrl(QUrl(api));
    req.setRawHeader("User-Agent", "XiaoQinTools");
    req.setRawHeader("Accept", "application/vnd.github+json");
    // follow 301/302 redirects (repo moved etc.)
    req.setMaximumRedirectsAllowed(5);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_mgr->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_lastError = "检查失败：" + reply->errorString();
            emit checkFinished(false);
            return;
        }
        parseLatestRelease(reply->readAll());
    });
}

void UpdateService::downloadAndInstall()
{
    if (m_downloading || m_url.isEmpty()) return;

    // target dir: %TEMP%/XiaoQinToolsUpdate
    QString dir = QDir::temp().filePath("XiaoQinToolsUpdate");
    QDir().mkpath(dir);
    QString fileName = QUrl(m_url).fileName();
    if (fileName.isEmpty()) fileName = "update.exe";
    QString dest = dir + "/" + fileName;

    // pick the fastest source (GitHub direct + mirrors), then download from it.
    // Run the speed probe off the UI thread so the window doesn't freeze.
    QStringList mirrors = mirrorUrls(m_url);
    m_downloading = true;
    m_progress = 0;
    emit downloadStateChanged();
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, dest]() {
        QString fast = watcher->result();
        watcher->deleteLater();
        if (fast.isEmpty())
            emit downloadFinished(false, "所有下载源均不可用，请检查网络或代理");
        else
            startDownload(fast, dest);
    });
    QFuture<QString> future = QtConcurrent::run([mirrors, this]() {
        return pickFastest(mirrors, 512 * 1024, 5000); // probe 512KB, 5s cap per source
    });
    watcher->setFuture(future);
}

// ---- build mirror URLs for a canonical GitHub release download URL ----
QStringList UpdateService::mirrorUrls(const QString &canonical) const
{
    QStringList out;
    out << canonical; // official GitHub first
    // public GitHub proxy mirrors (fastest one is picked by the speed test)
    const QStringList prefix = {
        "https://ghproxy.net/",
        "https://gh-proxy.com/",
        "https://mirror.ghproxy.com/",
        "https://ghfast.top/",
        "https://ghproxy.cc/",
    };
    for (const QString &p : prefix)
        out << p + canonical;
    return out;
}

// ---- probe each candidate URL with a small ranged request, return the fastest ----
QString UpdateService::pickFastest(const QStringList &urls, int probeBytes, int timeoutMs)
{
    // Runs on a worker thread: use a local manager (no parent) so we don't touch
    // the main-thread m_mgr from another thread.
    QNetworkAccessManager mgr;
    ProxyService probe;
    if (probe.isClashPortOpen())
        mgr.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 7897));
    else if (probe.isV2rayPortOpen())
        mgr.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 10808));
    else
        mgr.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));

    QString best;
    double bestSpeed = -1.0;
    for (const QString &u : urls) {
        QNetworkRequest req;
        req.setUrl(QUrl(u));
        req.setRawHeader("User-Agent", "XiaoQinTools");
        // request only the first probeBytes via a Range header
        req.setRawHeader("Range", QString("bytes=0-%1").arg(probeBytes - 1).toUtf8());

        QEventLoop loop;
        QElapsedTimer t;
        t.start();
        QNetworkReply *rep = mgr.get(req);
        qint64 got = 0;
        QObject::connect(rep, &QNetworkReply::readyRead, &loop, [&got, rep]() {
            got += rep->readAll().size();
        });
        QObject::connect(rep, &QNetworkReply::finished, &loop, [&loop, rep]() {
            rep->deleteLater();
            loop.quit();
        });
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();

        qint64 ms = t.elapsed();
        rep->abort();
        rep->deleteLater();
        if (ms <= 0 || got <= 0) continue; // unreachable or nothing received
        double speed = (double)got * 1000.0 / (double)ms; // bytes/sec
        if (speed > bestSpeed) {
            bestSpeed = speed;
            best = u;
        }
    }
    return best;
}

// ---- actually download from the chosen URL and finish the update flow ----
void UpdateService::startDownload(const QString &url, const QString &dest)
{
    m_downloading = true;
    m_progress = 0;
    emit downloadStateChanged();

    if (!m_mgr) m_mgr = new QNetworkAccessManager(this);
    ProxyService probe;
    if (probe.isClashPortOpen())
        m_mgr->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 7897));
    else if (probe.isV2rayPortOpen())
        m_mgr->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 10808));
    else
        m_mgr->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    QNetworkRequest req;
    req.setUrl(QUrl(url));
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
        emit downloadFinished(true, "更新包已下载，正在解压安装...");

        // zip: extract with system tar, replace the app dir, then relaunch self.
        // exe: just launch the installer.
        bool isZip = dest.endsWith(".zip", Qt::CaseInsensitive);
        if (!isZip) {
            QProcess::startDetached(dest, QStringList());
            QTimer::singleShot(1500, qApp, &QCoreApplication::quit);
            return;
        }

        // work in a staging dir to avoid partial replacement on failure
        QString staging = QDir::temp().filePath("XiaoQinToolsStage_" + QString::number(QCoreApplication::applicationPid()));
        QDir().mkpath(staging);
        QDir().mkpath(staging + "/new");

        // 1) extract zip into staging/new
        QProcess tar;
        tar.start("tar", QStringList() << "-xf" << dest << "-C" << staging + "/new");
        tar.waitForFinished(120000);
        if (tar.exitStatus() != QProcess::NormalExit || tar.exitCode() != 0) {
            emit downloadFinished(false, "解压失败：" + QString::fromLocal8Bit(tar.readAllStandardError()).left(120));
            return;
        }

        // 2) find the installer exe inside the zip and launch it
        QString setupExe;
        {
            QDirIterator it(staging + "/new", QStringList() << "XiaoQinTools-*-setup.exe" << "setup.exe",
                            QDir::Files, QDirIterator::Subdirectories);
            if (it.hasNext()) setupExe = it.next();
        }
        if (setupExe.isEmpty()) {
            emit downloadFinished(false, "更新包内容异常（未找到安装程序）");
            return;
        }

        // 3) launch the installer (silent), then exit so files can be replaced
        QProcess::startDetached(setupExe, QStringList() << "/VERYSILENT" << "/SUPPRESSMSGBOXES" << "/NORESTART");
        QTimer::singleShot(1500, qApp, &QCoreApplication::quit);
    });
}
