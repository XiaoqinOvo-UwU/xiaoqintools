#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QFont>
#include <QDir>
#include <QCoreApplication>
#include <QtQuickControls2/QQuickStyle>

#include "Core/AppCore.h"
#include "Services/ProxyService.h"
#include "Services/NetworkService.h"
#include "Services/SystemService.h"
#include "Services/MoodService.h"
#include "Services/ConfigService.h"
#include "Services/UpdateService.h"
#include "Services/SyncService.h"
#include "Services/PluginManager.h"
#include "Services/AiService.h"
#include "Services/StatsService.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("XiaoQin");
    app.setApplicationName("XiaoQinTools");
    app.setApplicationVersion(NIGHTLOG_VERSION);
    app.setWindowIcon(QIcon(":/icons/app.ico"));

    QFont f("Microsoft YaHei UI");
    f.setPixelSize(14);
    app.setFont(f);

    // Fusion style honors the dark palette set from QML -> dark TextFields/Groups.
    QQuickStyle::setStyle("Fusion");

    // register C++ services as QML types
    qmlRegisterType<ProxyService>("XiaoQin.Services", 1, 0, "ProxyService");
    qmlRegisterType<NetworkService>("XiaoQin.Services", 1, 0, "NetworkService");
    qmlRegisterType<SystemService>("XiaoQin.Services", 1, 0, "SystemService");
    qmlRegisterType<MoodService>("XiaoQin.Services", 1, 0, "MoodService");
    qmlRegisterType<UpdateService>("XiaoQin.Services", 1, 0, "UpdateService");
    qmlRegisterType<SyncService>("XiaoQin.Services", 1, 0, "SyncService");
    qmlRegisterType<PluginManager>("XiaoQin.Services", 1, 0, "PluginManager");
    qmlRegisterType<AiService>("XiaoQin.Services", 1, 0, "AiService");
    qmlRegisterType<StatsService>("XiaoQin.Services", 1, 0, "StatsService");

    QQmlApplicationEngine engine;
    // Make QML modules resolvable next to the executable (deployed Qt plugins).
    QString importPath = QCoreApplication::applicationDirPath() + "/qml";
    engine.addImportPath(importPath);
    engine.addImportPath("qrc:/");
    engine.rootContext()->setContextProperty("appCore", &AppCore::instance());
    engine.rootContext()->setContextProperty("proxyService", new ProxyService(&engine));
    engine.rootContext()->setContextProperty("netService", new NetworkService(&engine));
    engine.rootContext()->setContextProperty("sysService", new SystemService(&engine));
    engine.rootContext()->setContextProperty("moodService", new MoodService(&engine));
    engine.rootContext()->setContextProperty("updateService", new UpdateService(&engine));
    engine.rootContext()->setContextProperty("syncService", new SyncService(&engine));
    engine.rootContext()->setContextProperty("pluginManager", new PluginManager(&engine));
    auto *aiSvc = new AiService(&engine);
    engine.rootContext()->setContextProperty("aiService", aiSvc);
    aiSvc->recordSessionStart();
    engine.rootContext()->setContextProperty("statsService", new StatsService(&engine));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl("qrc:/qml/Main.qml"));
    return app.exec();
}
