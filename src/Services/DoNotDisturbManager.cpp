#include "DoNotDisturbManager.h"
#include <QStringList>

DoNotDisturbState DoNotDisturbManager::evaluate(const QString &fgCategory,
                                                const QString &fgExe,
                                                bool isFullscreen,
                                                bool isGameProcess)
{
    DoNotDisturbState s;
    Q_UNUSED(fgCategory); // (kept in the signature for clarity)

    const QString e = fgExe.toLower();

    // 1) gaming — the most important silence reason
    if (isGameProcess || fgCategory == "gaming") {
        s.enabled = true;
        s.reason = "gaming";
        return s;
    }
    // 2) meeting / voice-call software
    static const QStringList meetings = {
        "zoom", "teams", "wemeet", "腾讯会议", "dingtalk", "钉钉",
        "feishu", "skype", "webex", "voov", "meeting",
    };
    for (const QString &m : meetings)
        if (e.contains(m)) {
            s.enabled = true;
            s.reason = "meeting";
            return s;
        }
    // 3) fullscreen app (any) — covers fullscreen games/videos/immersive work
    if (isFullscreen) {
        s.enabled = true;
        s.reason = "fullscreen";
        return s;
    }
    // 4) high-intensity work: coding / creating / terminal
    if (fgCategory == "creating" || fgCategory == "terminal") {
        s.enabled = true;
        s.reason = "work";
        return s;
    }

    return s; // enabled stays false
}
