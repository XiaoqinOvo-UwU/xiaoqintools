#pragma once
#include <QString>

// =====================================================================
// DoNotDisturbManager — decides when the AI should stay silent.
//
// Auto-judged from real desktop state only (never guessed by the LLM):
//   - gaming
//   - fullscreen app
//   - meeting software (zoom / teams / 腾讯会议 / dingtalk ...)
//   - high-intensity work (coding / creating / terminal)
//
// Rules:
//   - proactive chat (idleChat) is BANNED while enabled
//   - user-initiated messages are ALWAYS answered normally
// =====================================================================

struct DoNotDisturbState
{
    bool    enabled = false;
    QString reason;          // "gaming" | "fullscreen" | "meeting" | "work" | ""
};

class DoNotDisturbManager
{
public:
    // `fgCategory` = AppAnalyzer category of the foreground app
    // ("gaming"/"creating"/"terminal"/...), `fgExe` = foreground process name,
    // `isFullscreen` = the foreground window covers the whole screen.
    static DoNotDisturbState evaluate(const QString &fgCategory,
                                      const QString &fgExe,
                                      bool isFullscreen,
                                      bool isGameProcess);
};
