#include "SyncService.h"
#include "ConfigService.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

SyncService::SyncService(QObject *parent)
    : QObject(parent)
{
}

QString SyncService::defaultExportPath()
{
    return "C:/XiaoQinData/tools-data配置.json";
}

bool SyncService::exportConfig(const QString &destPath)
{
    QJsonObject o;
    o.insert("base_url", ConfigService::instance().baseUrl());
    o.insert("model", ConfigService::instance().model());
    o.insert("api_key", ConfigService::instance().apiKey());
    o.insert("clash_path", ConfigService::instance().clashPath());
    o.insert("v2ray_path", ConfigService::instance().v2rayPath());
    o.insert("exported_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    QFile f(destPath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson());
    f.close();
    return true;
}

bool SyncService::importConfig(const QString &srcPath)
{
    QFile f(srcPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return false;
    QJsonObject o = doc.object();

    if (o.contains("base_url")) ConfigService::instance().setBaseUrl(o.value("base_url").toString());
    if (o.contains("model")) ConfigService::instance().setModel(o.value("model").toString());
    if (o.contains("api_key")) ConfigService::instance().setApiKey(o.value("api_key").toString());
    return true;
}
