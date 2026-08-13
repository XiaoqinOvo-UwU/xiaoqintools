#pragma once
#include <QObject>
#include <QString>
#include <QHash>
#include <QJsonArray>
class QTimer;

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
    Q_INVOKABLE void setChatHistory(const QString &history); // seed recent-chat context (from UI/SQLite)
    Q_INVOKABLE QString memoryReport();      // what the AI remembers (for settings/tests)
    Q_INVOKABLE QString uptimeText();        // how long the PC has been on
    Q_INVOKABLE void generateGreeting();     // async: DeepSeek writes greeting -> greetingReady
    Q_INVOKABLE bool isGameRunning();        // detects r5apex / minecraft etc.
    Q_INVOKABLE bool isFullscreenGame();     // foreground window covers the whole screen (any game)
    Q_INVOKABLE bool isForegroundMinecraft(); // foreground window is Minecraft
    Q_INVOKABLE void idleChat();             // async: AI initiates a topic -> chatReply
    Q_INVOKABLE qint64 lastInputMs();        // ms since the last keyboard/mouse input system-wide

    // ---- batch A: proactive conversation engine ----
    Q_INVOKABLE bool isUserBusy();           // true if coding/typing-heavy foreground
    Q_INVOKABLE QString userActivityState(); // "coding"/"gaming"/"idle"/"away"/"late_night"
    Q_INVOKABLE void trackUnfinishedTopic(const QString &topic, int importance); // save unfinished topic
    Q_INVOKABLE QString unfinishedTopicsText(int max); // recent unfinished topics for prompt
    Q_INVOKABLE void recordInterest(const QString &interest); // bump interest weight
    Q_INVOKABLE QString interestsText(int max); // top interests for prompt
    Q_INVOKABLE QString aiStateJson();       // {"mood":"calm","energy":80,...}
    Q_INVOKABLE void setAiMood(const QString &mood); // update AI mood (continuity)
    Q_INVOKABLE void adjustAiEnergy(int delta);      // energy after interactions

    // ---- AI state friendly accessors (Slider/ComboBox friendly) ----
    Q_INVOKABLE int tenderness();       // 0-100 (温柔程度)
    Q_INVOKABLE int energy();           // 0-100 (活跃程度)
    Q_INVOKABLE int companion();        // 0-100 (陪伴倾向)
    Q_INVOKABLE QString moodText();     // "calm"/"happy"/"gentle"/"low"/"active"
    Q_INVOKABLE QString replyStyle();   // "short"/"natural"/"detailed"
    Q_INVOKABLE void setTenderness(int v);
    Q_INVOKABLE void setEnergy(int v);
    Q_INVOKABLE void setCompanion(int v);
    Q_INVOKABLE void setMoodText(const QString &m);
    Q_INVOKABLE void setReplyStyle(const QString &s);

    // ---- batch B: personality + relationship ----
    Q_INVOKABLE QString personalityText();   // structured traits/style from personality.json
    Q_INVOKABLE QString personalityRaw();    // full personality.json as text (for editing)
    Q_INVOKABLE void setPersonalityRaw(const QString &json); // replace personality.json
    Q_INVOKABLE QString relationshipText();  // relationship continuity for prompt
    Q_INVOKABLE void bumpRelationship(int intimacyDelta, int trustDelta);
    Q_INVOKABLE void setRelationship(int intimacy, int trust); // manual override
    Q_INVOKABLE QString interestsRaw();      // interests.json as text
    Q_INVOKABLE void setInterestsRaw(const QString &json);
    Q_INVOKABLE QString unfinishedRaw();     // unfinished_topics.json as text
    Q_INVOKABLE void setUnfinishedRaw(const QString &json);
    Q_INVOKABLE void setAiEnergy(int energy); // manual energy override
    Q_INVOKABLE QString notesText();        // user notes only
    Q_INVOKABLE QString usageText();        // usage duration only

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

    // ---- companion: relationship state + event memory (batch 1) ----
    Q_INVOKABLE void recordEvent(const QString &type, const QString &summary); // event memory (with date)
    Q_INVOKABLE QString eventMemoryText(int maxEvents); // recent events for prompt injection

    Q_INVOKABLE QString foregroundApp();                 // current foreground window title (lightweight)

    // ---- batch 2: system state sensing + analysis ----
    Q_INVOKABLE QString systemStateJson();               // {cpu, mem, hour, foreground, idleMs, ...}
    Q_INVOKABLE QString analyzeUserState();              // human state: gaming/working/late_night/away/relaxed
    Q_INVOKABLE QString batteryState();                  // battery percent + plugged (or empty on desktop)

    // ---- batch 3: emotion + response strategy ----
    Q_INVOKABLE QString inferUserEmotion(const QString &text); // happy/tired/stressed/lonely/normal

    // privacy toggles (forwarded to ConfigService for QML access)
    Q_INVOKABLE bool allowStateRead();
    Q_INVOKABLE bool allowTimeRecord();
    Q_INVOKABLE bool allowLongTermMemory();
    Q_INVOKABLE void setAllowStateRead(bool v);
    Q_INVOKABLE void setAllowTimeRecord(bool v);
    Q_INVOKABLE void setAllowLongTermMemory(bool v);

    // ---- PC activity monitor (startup/shutdown + software usage) ----
    Q_INVOKABLE void startActivityMonitor();   // begin periodic foreground-app sampling
    Q_INVOKABLE void stopActivityMonitor();
    Q_INVOKABLE QString activitySummary();     // "今天大部分时间在用什么" (from short-term memory)

signals:
    void chatReply(QString text);
    void idleReply(QString text);            // AI initiated a chat on its own (proactive)
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
    static QString callDeepSeekMessages(const QJsonArray &messages);
    QString jsonGet(const QString &json, const QString &key);
    QString jsonSet(const QString &json, const QString &key, const QString &value);

    // auto memory: after N user turns, summarize the recent chat into a short note
    void trackChatTurn(const QString &userText, const QString &aiReply);
    void maybeSummarize();
    void appendNote(const QString &note);
    QStringList m_chatBuffer;   // recent turns (user/ai pairs), bounded
    int m_userTurns = 0;        // user messages since last summary
    bool m_summarizing = false;

    // activity monitor internals
    void recordActivitySample();      // called by the timer
    void flushActivityToMemory();     // write the accumulated usage into memory
    QHash<QString, int> m_appMinutes; // app title -> minutes (today)
    QTimer *m_monitorTimer = nullptr;
};
