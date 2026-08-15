#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QFont>
#include <QDir>
#include <QCoreApplication>
#include <QtQuickControls2/QQuickStyle>
#include <QQuickWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

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
#include "Services/ContactService.h"

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
    qmlRegisterType<ContactService>("XiaoQin.Services", 1, 0, "ContactService");

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
    aiSvc->startActivityMonitor();
    engine.rootContext()->setContextProperty("statsService", new StatsService(&engine));
    engine.rootContext()->setContextProperty("contactService", &ContactService::instance());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(QUrl("qrc:/qml/Main.qml"));

#ifdef Q_OS_WIN
    // frameless: ask DWM to round the window corners natively (Windows 11).
    // The window itself stays OPAQUE, so DWM's corner clip produces real
    // rounded corners with the desktop showing through — no QML shaders needed.
    if (!engine.rootObjects().isEmpty()) {
        if (auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
            HWND hwnd = reinterpret_cast<HWND>(win->winId());
            const DWORD round = 2; // DWMWCP_ROUND
            DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/,
                                  &round, sizeof(round));
        }
    }
#endif

    QObject::connect(&app, &QCoreApplication::aboutToQuit, aiSvc, [aiSvc]() {
        aiSvc->recordSessionEnd();
        aiSvc->stopActivityMonitor();
    });
    return app.exec();
}
