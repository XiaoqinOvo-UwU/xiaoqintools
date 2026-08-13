#include "ConfigService.h"

#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QCoreApplication>
#include <QSettings>

ConfigService &ConfigService::instance()
{
    static ConfigService inst;
    return inst;
}

QString ConfigService::configDir() const
{
    // Keep the "杂货铺" concept alive: settings live under AppData\XiaoQinTools
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir d(base);
    if (!d.exists()) d.mkpath(".");
    return base;
}

QString ConfigService::configPath() const
{
    return configDir() + "/config.json";
}

void ConfigService::load()
{
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    QJsonObject o = doc.object();
    if (o.contains("base_url")) m_baseUrl = o.value("base_url").toString();
    if (o.contains("model")) m_model = o.value("model").toString();
    if (o.contains("api_key")) m_apiKey = o.value("api_key").toString();
    if (o.contains("clash_path")) m_clashPath = o.value("clash_path").toString();
    if (o.contains("v2ray_path")) m_v2rayPath = o.value("v2ray_path").toString();
    if (o.contains("user_name")) m_userName = o.value("user_name").toString();
    if (o.contains("avatar_char")) m_avatarChar = o.value("avatar_char").toString();
    if (o.contains("ai_name")) m_aiName = o.value("ai_name").toString();
    if (o.contains("ai_personality")) m_aiPersonality = o.value("ai_personality").toString();
    if (o.contains("allow_state_read")) m_allowStateRead = o.value("allow_state_read").toBool(true);
    if (o.contains("allow_time_record")) m_allowTimeRecord = o.value("allow_time_record").toBool(true);
    if (o.contains("allow_long_term_memory")) m_allowLongTermMemory = o.value("allow_long_term_memory").toBool(true);
    f.close();
}

void ConfigService::save()
{
    QJsonObject o;
    o.insert("base_url", m_baseUrl);
    o.insert("model", m_model);
    o.insert("api_key", m_apiKey);
    o.insert("clash_path", m_clashPath);
    o.insert("v2ray_path", m_v2rayPath);
    o.insert("user_name", m_userName);
    o.insert("avatar_char", m_avatarChar);
    o.insert("ai_name", m_aiName);
    o.insert("ai_personality", m_aiPersonality);
    o.insert("allow_state_read", m_allowStateRead);
    o.insert("allow_time_record", m_allowTimeRecord);
    o.insert("allow_long_term_memory", m_allowLongTermMemory);
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(o).toJson());
    f.close();
}

QString ConfigService::detectClashExe() const
{
    const QStringList candidates = {
        "C:/Program Files/Clash Verge/clash-verge.exe",
        "C:/Program Files (x86)/Clash Verge/clash-verge.exe",
        "D:/Program Files/Clash Verge/clash-verge.exe",
        "D:/Clash Verge/clash-verge.exe",
        "E:/Clash Verge/clash-verge.exe",
        QDir::home().filePath("AppData/Local/Programs/clash-verge/clash-verge.exe"),
        QDir::home().filePath("AppData/Local/Clash Verge/clash-verge.exe"),
        QDir::home().filePath("Downloads/Clash Verge/clash-verge.exe"),
        QDir::home().filePath("Desktop/Clash Verge/clash-verge.exe"),
        "C:/Clash Verge/clash-verge.exe",
    };
    for (const QString &c : candidates)
        if (QFile::exists(c)) return c;

    // registry uninstall entries (HKCU + HKLM, both 64/32 views)
    const QStringList keys = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };
    for (const QString &k : keys) {
        QSettings s(k, QSettings::NativeFormat);
        for (const QString &sub : s.childGroups()) {
            s.beginGroup(sub);
            QString disp = s.value("DisplayName").toString();
            QString loc = s.value("InstallLocation").toString();
            QString exe = s.value("DisplayIcon").toString();
            s.endGroup();
            if (disp.contains("Clash", Qt::CaseInsensitive) || exe.contains("clash-verge", Qt::CaseInsensitive)) {
                // try install location first, then the DisplayIcon path
                QString p = loc + "/clash-verge.exe";
                if (QFile::exists(p)) return p;
                QString p2 = loc + "/Clash Verge.exe";
                if (QFile::exists(p2)) return p2;
                QString icon = exe.section(',', 0, 0);
                if (QFile::exists(icon)) return icon;
            }
        }
    }

    // portable Clash Verge (green build) fallback: search common roots
    const QStringList clashRoots = {
        QDir::homePath() + "/Desktop",
        QDir::homePath() + "/Desktop/梯子",
        QDir::homePath() + "/Downloads",
        "C:/",
        "D:/",
        "E:/",
    };
    for (const QString &root : clashRoots) {
        QDir dir(root);
        if (!dir.exists()) continue;
        const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &d : entries) {
            if (d.fileName().contains("clash", Qt::CaseInsensitive)) {
                QString p1 = d.absoluteFilePath() + "/clash-verge.exe";
                if (QFile::exists(p1)) return p1;
                QString p2 = d.absoluteFilePath() + "/Clash Verge.exe";
                if (QFile::exists(p2)) return p2;
                // portable Rev: clash-verge folder may contain a nested app dir
                QDir sub(d.absoluteFilePath());
                const auto subs = sub.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QFileInfo &sd : subs) {
                    QString deeper = sd.absoluteFilePath() + "/clash-verge.exe";
                    if (QFile::exists(deeper)) return deeper;
                }
            }
        }
    }
    return {};
}

QString ConfigService::detectV2rayExe() const
{
    const QString desk = QDir::homePath() + "/Desktop";
    const QString downloads = QDir::homePath() + "/Downloads";
    const QStringList candidates = {
        desk + "/梯子/v2rayN-windows-64-SelfContained/v2rayN.exe",
        desk + "/v2rayN-windows-64-SelfContained/v2rayN.exe",
        desk + "/v2rayN/v2rayN.exe",
        desk + "/梯子/v2rayN/v2rayN.exe",
        desk + "/梯子/v2rayN-windows-64/v2rayN.exe",
        downloads + "/v2rayN-windows-64-SelfContained/v2rayN.exe",
        downloads + "/v2rayN/v2rayN.exe",
        downloads + "/梯子/v2rayN-windows-64-SelfContained/v2rayN.exe",
        "C:/v2rayN/v2rayN.exe",
        "C:/Program Files/v2rayN/v2rayN.exe",
        "C:/Program Files (x86)/v2rayN/v2rayN.exe",
        "D:/v2rayN/v2rayN.exe",
        "D:/Program Files/v2rayN/v2rayN.exe",
        "E:/v2rayN/v2rayN.exe",
        QDir::home().filePath("AppData/Local/Programs/v2rayN/v2rayN.exe"),
    };
    for (const QString &c : candidates)
        if (QFile::exists(c)) return c;

    // registry uninstall entries
    const QStringList keys = {
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };
    for (const QString &k : keys) {
        QSettings s(k, QSettings::NativeFormat);
        for (const QString &sub : s.childGroups()) {
            s.beginGroup(sub);
            QString disp = s.value("DisplayName").toString();
            QString loc = s.value("InstallLocation").toString();
            QString exe = s.value("DisplayIcon").toString();
            s.endGroup();
            if (disp.contains("v2ray", Qt::CaseInsensitive) || exe.contains("v2rayN", Qt::CaseInsensitive)) {
                QString p = loc + "/v2rayN.exe";
                if (QFile::exists(p)) return p;
                QString icon = exe.section(',', 0, 0);
                if (QFile::exists(icon)) return icon;
            }
        }
    }

    // v2rayN is commonly a green/portable build without registry entry:
    // search likely roots up to 3 levels deep, skipping huge dirs.
    const QStringList roots = {
        QDir::homePath() + "/Desktop",
        QDir::homePath() + "/Downloads",
        QDir::homePath() + "/Documents",
        QDir::homePath() + "/Desktop/梯子",
        "C:/",
        "D:/",
        "E:/",
    };
    for (const QString &root : roots) {
        QDir dir(root);
        if (!dir.exists()) continue;
        const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &d : entries) {
            if (d.fileName().contains("v2ray", Qt::CaseInsensitive)) {
                QString direct = d.absoluteFilePath() + "/v2rayN.exe";
                if (QFile::exists(direct)) return direct;
                // one level deeper (e.g. v2rayN-windows-64-SelfContained)
                QDir sub(d.absoluteFilePath());
                const auto subs = sub.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
                for (const QFileInfo &sd : subs) {
                    QString deeper = sd.absoluteFilePath() + "/v2rayN.exe";
                    if (QFile::exists(deeper)) return deeper;
                }
            }
        }
    }
    return {};
}
