#pragma once
#include <QObject>
#include <QString>

// Config sync: export current settings to a JSON file (or remote later),
// and import from one. Keeps the 杂货铺 convention for local backups.
class SyncService : public QObject
{
    Q_OBJECT
public:
    explicit SyncService(QObject *parent = nullptr);

    Q_INVOKABLE bool exportConfig(const QString &destPath);
    Q_INVOKABLE bool importConfig(const QString &srcPath);
    Q_INVOKABLE QString defaultExportPath(); // e.g. C:/XiaoQinData/tools-data配置.json
};
