#pragma once
#include <QObject>
#include <QString>

// AI companion card: chat with DeepSeek + long-term memory.
// Memory: JSON file storing session times, novel-edit counts, sleep habits.
// On morning first launch, generates a greeting like:
//   "早上好，用户。昨天你的OpenCode改代码8小时。你的小说文档昨天修改了三次。另外……你又凌晨三点睡觉。"
class AiService : public QObject
{
    Q_OBJECT
public:
    explicit AiService(QObject *parent = nullptr);

    Q_INVOKABLE QString greeting();          // morning greeting from memory
    Q_INVOKABLE bool shouldGreetToday();     // only greet on the first launch of the day
    Q_INVOKABLE void markGreeted();          // record today as greeted
    Q_INVOKABLE void sendMessage(const QString &text); // async chat -> chatReply
    Q_INVOKABLE QString memoryReport();      // what the AI remembers (for settings/tests)
    Q_INVOKABLE QString uptimeText();        // how long the PC has been on
    Q_INVOKABLE void generateGreeting();     // async: DeepSeek writes greeting -> greetingReady
    Q_INVOKABLE bool isGameRunning();        // detects r5apex / minecraft etc.
    Q_INVOKABLE void idleChat();             // async: AI initiates a topic -> chatReply

    // user profile passthrough (stored in config)
    Q_INVOKABLE QString userName();
    Q_INVOKABLE QString avatarChar();
    Q_INVOKABLE void setUserName(const QString &v);
    Q_INVOKABLE void setAvatarChar(const QString &v);

    // current contact (AI) profile — delegated to ContactService
    Q_INVOKABLE QString aiName();
    Q_INVOKABLE QString aiPersonality();
    Q_INVOKABLE void setAiName(const QString &v);
    Q_INVOKABLE void setAiPersonality(const QString &v);
    Q_INVOKABLE QString setAiAvatar(const QString &srcPath);
    Q_INVOKABLE QString aiAvatarPath();

    // AI API config passthrough
    Q_INVOKABLE QString apiBaseUrl();
    Q_INVOKABLE QString apiModel();
    Q_INVOKABLE QString apiKey();
    Q_INVOKABLE void setApiBaseUrl(const QString &v);
    Q_INVOKABLE void setApiModel(const QString &v);
    Q_INVOKABLE void setApiKey(const QString &v);

    // avatar images: copy a local image into app data, return stored path
    Q_INVOKABLE QString setUserAvatar(const QString &srcPath);
    Q_INVOKABLE QString userAvatarPath();

    // lifecycle: call on app start / exit to record sessions
    Q_INVOKABLE void recordSessionStart();
    Q_INVOKABLE void recordSessionEnd();

    // ---- AIRI-style memory management ----
    Q_INVOKABLE QString memoryDetail();                 // full human-readable memory (settings view)
    Q_INVOKABLE void addMemoryNote(const QString &note); // user tells AI something to remember
    Q_INVOKABLE void clearMemory();                      // wipe memory (keep created stamp)

    Q_INVOKABLE QString foregroundApp();                 // current foreground window title (lightweight)

signals:
    void chatReply(QString text);
    void thinkingReady(QString text);
    void greetingReady(QString text);
    void emotionSignal(QString emotion, qreal intensity); // AIRI-style ACT token playback
    void profileChanged();                               // name/avatar/persona changed -> refresh UI

private:
    QString memoryPath() const;
    void ensureMemory();
    QString readMemory() const;
    void writeMemory(const QString &json);
    static QString callDeepSeekStatic(const QString &system, const QString &user);
    QString jsonGet(const QString &json, const QString &key);
    QString jsonSet(const QString &json, const QString &key, const QString &value);

    // auto memory: after N user turns, summarize the recent chat into a short note
    void trackChatTurn(const QString &userText, const QString &aiReply);
    void maybeSummarize();
    void appendNote(const QString &note);
    QStringList m_chatBuffer;   // recent turns (user/ai pairs), bounded
    int m_userTurns = 0;        // user messages since last summary
    bool m_summarizing = false;
};
