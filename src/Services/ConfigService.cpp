#include "ConfigService.h"

#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QCoreApplication>

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
        QDir::home().filePath("AppData/Local/Programs/clash-verge/clash-verge.exe"),
        "D:/Clash Verge/clash-verge.exe",
    };
    for (const QString &c : candidates)
        if (QFile::exists(c)) return c;
    return {};
}

QString ConfigService::detectV2rayExe() const
{
    const QString desk = QDir::homePath() + "/Desktop";
    const QStringList candidates = {
        desk + "/梯子/v2rayN-windows-64-SelfContained/v2rayN.exe",
        desk + "/v2rayN-windows-64-SelfContained/v2rayN.exe",
        desk + "/v2rayN/v2rayN.exe",
        "C:/v2rayN/v2rayN.exe",
        "D:/v2rayN/v2rayN.exe",
    };
    for (const QString &c : candidates)
        if (QFile::exists(c)) return c;
    return {};
}
