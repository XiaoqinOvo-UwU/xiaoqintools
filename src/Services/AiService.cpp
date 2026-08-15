#include "AiService.h"
#include "ConfigService.h"
#include "ContactService.h"
#include "ContextManager.h"
#include "FactFilter.h"
#include "ResponseValidator.h"
#include "ConversationState.h"
#include "MemoryRetriever.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QRegularExpression>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QTimer>
#include <QMap>
#include <algorithm>

// forward decls for helpers defined later in this file
static QJsonObject readAiState();
static QString unfinishedPath();
static QString interestPath();
static QJsonObject readRelationship();

AiService::AiService(QObject *parent)
    : QObject(parent)
    , m_ctx(new ContextManager)
    , m_convo(new ConversationState)
{
    ensureMemory();
}

QString AiService::memoryPath() const
{
    // per-contact memory lives in %APPDATA% so it survives deleting the tool directory
    return ContactService::instance().contactMemoryPath(ContactService::instance().currentId());
}

void AiService::ensureMemory()
{
    QDir().mkpath(ContactService::instance().contactDir(ContactService::instance().currentId()));

    // migrate legacy memory files if present (first-run only, default contact)
    if (ContactService::instance().currentId().isEmpty()) {
        const QStringList legacy = {
            "C:/XiaoQinData/用户工具/ai_memory.json",
            "C:/deepseek杂货铺/小钦工具/ai_memory.json",
            "C:/XiaoQinData/tools-data/ai_memory.json",
        };
        if (!QFile::exists(memoryPath())) {
            for (const QString &old : legacy) {
                if (QFile::exists(old)) {
                    QFile::copy(old, memoryPath());
                    break;
                }
            }
        }
    }

    if (!QFile::exists(memoryPath())) {
        QFile f(memoryPath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QJsonObject o;
            o.insert("created", QDateTime::currentDateTime().toString(Qt::ISODate));
            f.write(QJsonDocument(o).toJson());
            f.close();
        }
    }
}

QString AiService::readMemory() const
{
    QFile f(memoryPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return "{}";
    QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s.isEmpty() ? "{}" : s;
}

void AiService::writeMemory(const QString &json)
{
    QFile f(memoryPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(json.toUtf8());
        f.close();
    }
}

QString AiService::jsonGet(const QString &json, const QString &key)
{
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    if (!d.isObject()) return QString();
    return d.object().value(key).toString();
}

QString AiService::jsonSet(const QString &json, const QString &key, const QString &value)
{
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    o.insert(key, value);
    return QString::fromUtf8(QJsonDocument(o).toJson());
}

// ---- session recording ----
void AiService::recordSessionStart()
{
    if (!ConfigService::instance().allowTimeRecord()) return;
    QString mem = readMemory();
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    // count yesterday's sessions for activity duration
    QDateTime now = QDateTime::currentDateTime();
    QString lastEnd = jsonGet(mem, "lastSessionEnd");
    if (!lastEnd.isEmpty()) {
        QDateTime le = QDateTime::fromString(lastEnd, Qt::ISODate);
        if (le.isValid()) {
            // if last session ended in early morning, remember sleep habit
            if (le.time().hour() >= 23 || le.time().hour() < 3) {
                mem = jsonSet(mem, "sleepHabit", "凌晨" + QString::number(le.time().hour() + 1) + "点");
            }
            // shutdown/rest detection: long gap since the last session end means
            // the PC was off (or the user was away). Write a short-term memory note.
            qint64 gapHours = le.secsTo(now) / 3600;
            if (gapHours >= 4) {
                QString note = QString("用户休息了约 %1 小时（上次会话结束于 %2，本次开机 %3）")
                        .arg(gapHours)
                        .arg(le.toString("MM-dd HH:mm"))
                        .arg(now.toString("MM-dd HH:mm"));
                appendNote(note);
                mem = readMemory(); // appendNote rewrote the file
            }
        }
    }
    mem = jsonSet(mem, "lastSessionStart", now.toString(Qt::ISODate));
    mem = jsonSet(mem, "lastDate", today);
    writeMemory(mem);
}

void AiService::recordSessionEnd()
{
    if (!ConfigService::instance().allowTimeRecord()) return;
    QString mem = readMemory();
    QString end = QDateTime::currentDateTime().toString(Qt::ISODate);
    mem = jsonSet(mem, "lastSessionEnd", end);

    // flush any tracked app usage into memory before exit
    flushActivityToMemory();

    // accumulate usage minutes for today
    QString start = jsonGet(mem, "lastSessionStart");
    if (!start.isEmpty()) {
        QDateTime st = QDateTime::fromString(start, Qt::ISODate);
        if (st.isValid()) {
            qint64 mins = st.secsTo(QDateTime::currentDateTime()) / 60;
            QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            QString totalKey = "usageMinutes_" + today;
            int total = jsonGet(mem, totalKey).toInt() + (int)mins;
            mem = jsonSet(mem, totalKey, QString::number(total));
        }
    }
    writeMemory(mem);
}

// ---- PC activity monitor ----
void AiService::startActivityMonitor()
{
    if (m_monitorTimer) return; // already running
    if (!ConfigService::instance().allowTimeRecord()) return; // privacy: no usage tracking
    m_monitorTimer = new QTimer(this);
    m_monitorTimer->setInterval(60000); // sample every minute
    connect(m_monitorTimer, &QTimer::timeout, this, [this]() { recordActivitySample(); });
    m_monitorTimer->start();
    recordActivitySample(); // immediate first sample
}

void AiService::stopActivityMonitor()
{
    if (m_monitorTimer) {
        m_monitorTimer->stop();
        m_monitorTimer->deleteLater();
        m_monitorTimer = nullptr;
    }
    flushActivityToMemory();
}

void AiService::recordActivitySample()
{
    QString app = foregroundApp();
    if (app.isEmpty()) return;
    m_appMinutes[app] = m_appMinutes.value(app) + 1;
    // persist every 10 minutes so a crash doesn't lose everything
    if ((m_appMinutes.size() % 10) == 0 || m_appMinutes.value(app) >= 10)
        flushActivityToMemory();
}

void AiService::flushActivityToMemory()
{
    if (m_appMinutes.isEmpty()) return;
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();

    // merge into "appUsageToday" as "title: minutes" pairs
    QJsonObject usage = o.value("appUsageToday").toObject();
    for (auto it = m_appMinutes.constBegin(); it != m_appMinutes.constEnd(); ++it) {
        int existing = usage.value(it.key()).toInt();
        usage.insert(it.key(), existing + it.value());
    }
    o.insert("appUsageToday", usage);
    // keep only today's usage (reset key each day)
    o.insert("appUsageDate", QDate::currentDate().toString("yyyy-MM-dd"));
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
    m_appMinutes.clear();
}

QString AiService::activitySummary()
{
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QJsonObject usage = o.value("appUsageToday").toObject();
    if (usage.isEmpty()) return "（还没有检测到使用数据）";

    // normalize common apps to friendly labels, then pick top 3 by minutes
    QMap<int, QString> sorted; // minutes -> label (sorted ascending)
    for (auto it = usage.constBegin(); it != usage.constEnd(); ++it) {
        QString label = it.key();
        int mins = it.value().toInt();
        if (mins < 3) continue; // ignore < 3 min blips
        QString lower = label.toLower();
        if (lower.contains("edge") || lower.contains("chrome") || lower.contains("firefox"))
            label = "浏览器";
        else if (lower.contains("minecraft") || lower.contains("javaw") || lower.contains("java"))
            label = "Minecraft";
        else if (lower.contains("apex") || lower.contains("r5apex"))
            label = "Apex 英雄";
        else if (lower.contains("微信") || lower.contains("qq") || lower.contains("discord"))
            label = "聊天软件";
        else if (lower.contains("code") || lower.contains("visual studio"))
            label = "写代码";
        sorted.insert(mins, label);
    }
    if (sorted.isEmpty()) return "（今天还没怎么用电脑）";
    QStringList top;
    auto it = sorted.constEnd();
    for (int i = 0; i < 3 && it != sorted.constBegin(); ++i) {
        --it;
        top << QString("%1（%2 分钟）").arg(it.value()).arg(it.key());
    }
    return "今天大部分时间在：" + top.join("、");
}

// ---- novel edit counting (scan C:\AI库\小说 for yesterday-modified .md files) ----
static int countNovelEditsYesterday()
{
    QDir dir("C:/AI库/小说");
    if (!dir.exists()) return 0;
    QDate yesterday = QDate::currentDate().addDays(-1);
    int count = 0;
    const auto files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : files) {
        if (fi.suffix().compare("md", Qt::CaseInsensitive) == 0 && fi.lastModified().date() == yesterday)
            count++;
    }
    return count;
}

// ---- daily greeting gate (once per day) ----
bool AiService::shouldGreetToday()
{
    QString mem = readMemory();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    return jsonGet(mem, "lastGreetDate") != today;
}

void AiService::markGreeted()
{
    QString mem = readMemory();
    mem = jsonSet(mem, "lastGreetDate", QDate::currentDate().toString("yyyy-MM-dd"));
    writeMemory(mem);
}

// ---- greeting ----
QString AiService::greeting()
{
    QString mem = readMemory();
    QDateTime now = QDateTime::currentDateTime();
    int hour = now.time().hour();
    QString uname = ConfigService::instance().userName();
    if (uname.isEmpty()) uname = "用户";

    QStringList lines;
    if (hour >= 5 && hour < 11)
        lines << "早上好，" + uname + "。";
    else if (hour >= 11 && hour < 14)
        lines << "中午好，" + uname + "。";
    else if (hour >= 14 && hour < 18)
        lines << "下午好，" + uname + "。";
    else if (hour >= 18 && hour < 23)
        lines << "晚上好，" + uname + "。";
    else
        lines << "这么晚了还没睡……" + uname + "？";

    // yesterday usage minutes
    QDate yesterday = QDate::currentDate().addDays(-1);
    QString key = "usageMinutes_" + yesterday.toString("yyyy-MM-dd");
    int mins = jsonGet(mem, key).toInt();
    if (mins > 0) {
        int h = mins / 60, m = mins % 60;
        if (h >= 1) lines << QString("昨天你使用电脑 %1 小时 %2 分钟。").arg(h).arg(m);
        else lines << QString("昨天你使用电脑 %1 分钟。").arg(m);
    }

    // novel edits yesterday
    int edits = countNovelEditsYesterday();
    if (edits > 0)
        lines << QString("你的小说文档昨天修改了 %1 次。").arg(edits);

    // sleep habit
    QString sleepHabit = jsonGet(mem, "sleepHabit");
    if (!sleepHabit.isEmpty())
        lines << "另外……你又" + sleepHabit + "睡觉。";

    // system uptime
    QString up = uptimeText();
    if (!up.isEmpty())
        lines << up;

    if (lines.size() == 1) // nothing remembered yet
        lines << "今天也要元气满满哦 Ovo";

    return lines.join("\n");
}

// ---- memory report (human-readable) ----
QString AiService::memoryReport()
{
    QJsonDocument d = QJsonDocument::fromJson(readMemory().toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QStringList lines;

    QString created = o.value("created").toString();
    if (!created.isEmpty())
        lines << "首次使用：" + created.replace('T', ' ').left(16);

    QString lastDate = o.value("lastDate").toString();
    if (!lastDate.isEmpty())
        lines << "最近使用：" + lastDate;

    // sleep habit
    QString sleep = o.value("sleepHabit").toString();
    if (!sleep.isEmpty())
        lines << "睡眠习惯：" + sleep + "睡觉";

    // usage minutes for recent days
    lines << "";
    lines << "每日使用时长：";
    int shown = 0;
    for (int i = 3; i >= 0; i--) {
        QString day = QDate::currentDate().addDays(-i).toString("yyyy-MM-dd");
        QString key = "usageMinutes_" + day;
        int mins = o.value(key).toInt();
        if (mins > 0) {
            int h = mins / 60, m = mins % 60;
            lines << (h > 0 ? QString("  %1：%2 小时 %3 分钟").arg(day).arg(h).arg(m)
                            : QString("  %1：%2 分钟").arg(day).arg(m));
            shown++;
        }
    }
    if (shown == 0)
        lines << "  （暂无时长记录）";

    return lines.join("\n");
}

// ---- system uptime (Windows) ----
#ifdef Q_OS_WIN
#include <windows.h>
#endif
QString AiService::uptimeText()
{
#ifdef Q_OS_WIN
    ULONGLONG ms = GetTickCount64();
    qint64 secs = (qint64)(ms / 1000);
    qint64 h = secs / 3600, m = (secs % 3600) / 60;
    return QString("电脑已开机 %1 小时 %2 分钟。").arg(h).arg(m);
#else
    return QString();
#endif
}

// ---- current foreground window (lightweight, no process scan) ----
QString AiService::foregroundApp()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return QString();
    wchar_t buf[512];
    int len = GetWindowTextW(hwnd, buf, 512);
    if (len <= 0) return QString();
    QString t = QString::fromWCharArray(buf, len).trimmed();
    if (t.isEmpty()) return QString();
    // skip our own window title so the AI doesn't think it's "小钦的工具"
    if (t.contains("小钦的工具")) return QString();
    return t;
#else
    return QString();
#endif
}

// ---- idle detection: game running? ----
bool AiService::isGameRunning()
{
    // common games the user plays — NOTE: steam.exe is a background client,
    // not a game; treating it as "gaming" blocked idle chat entirely
    QStringList games = { "r5apex.exe", "javaw.exe", "java.exe", "Minecraft" };
    QProcess p;
    p.start("tasklist", QStringList() << "/FO" << "CSV");
    if (!p.waitForFinished(1500)) return false;
    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    for (const QString &g : games)
        if (out.contains(g, Qt::CaseInsensitive)) return true;
    return false;
}

// ---- is the foreground window a fullscreen game? (any game, not just a known list) ----
bool AiService::isFullscreenGame()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    if (!IsWindowVisible(hwnd)) return false;

    // ignore our own window
    DWORD ourPid = GetCurrentProcessId();
    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    if (winPid == ourPid) return false;

    RECT r;
    if (!GetClientRect(hwnd, &r)) return false;
    int w = r.right - r.left;
    int h = r.bottom - r.top;
    // treat as fullscreen when it covers the primary screen (>= 98% of each dimension)
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    return w >= sw * 98 / 100 && h >= sh * 98 / 100;
#else
    return false;
#endif
}

// ---- is the foreground window Minecraft? ----
// MC Java (including Chinese-named mod packs) runs on javaw.exe/java.exe,
// so we detect by the foreground window's process rather than the title alone.
bool AiService::isForegroundMinecraft()
{
#ifdef Q_OS_WIN
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) return false;
    if (!IsWindowVisible(hwnd)) return false;
    DWORD ourPid = GetCurrentProcessId();
    DWORD winPid = 0;
    GetWindowThreadProcessId(hwnd, &winPid);
    if (winPid == ourPid) return false;

    // check the foreground window's process name first
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, winPid);
    if (hProc) {
        wchar_t pbuf[512];
        DWORD n = 512;
        if (QueryFullProcessImageNameW(hProc, 0, pbuf, &n)) {
            QString path = QString::fromWCharArray(pbuf, (int)n);
            QString base = QFileInfo(path).fileName().toLower();
            CloseHandle(hProc);
            if (base == "javaw.exe" || base == "java.exe")
                return true; // Java game foreground -> treat as Minecraft
        } else {
            CloseHandle(hProc);
        }
    }

    // fallback: window title contains "Minecraft" (vanilla or english launchers)
    wchar_t buf[512];
    int len = GetWindowTextW(hwnd, buf, 512);
    if (len <= 0) return false;
    QString t = QString::fromWCharArray(buf, len).trimmed();
    return t.contains("Minecraft", Qt::CaseInsensitive);
#else
    return false;
#endif
}

// ---- system-wide idle: ms since the last keyboard/mouse input ----
qint64 AiService::lastInputMs()
{
#ifdef Q_OS_WIN
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (!GetLastInputInfo(&lii)) return -1;
    // GetTickCount wraps at ~49 days; guard against a stale read
    DWORD now = GetTickCount();
    DWORD delta = now - lii.dwTime; // unsigned wraps safely
    if (delta > 3600u * 24 * 1000) return -1; // > 1 day: treat as unknown
    return static_cast<qint64>(delta);
#else
    return -1;
#endif
}

// ---- idle chat: AI proactively starts a topic (topic-scored) ----
void AiService::idleChat()
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ContactService::instance().currentName();
    QString fg = foregroundApp();         // cheap win32 call
    QString recent = m_chatBuffer.join("\n"); // last few messages if any

    // proactive engine inputs: state, unfinished topics, interests, events
    QString state = userActivityState();
    QString aiState = aiStateJson();
    QStringList unfinished = unfinishedTopicsText(5).split('\n', Qt::SkipEmptyParts);
    QStringList interests = interestsText(5).split('\n', Qt::SkipEmptyParts);
    QStringList events = eventMemoryText(4).split('\n', Qt::SkipEmptyParts);
    QStringList recentLines = recent.split('\n', Qt::SkipEmptyParts);
    QStringList userFacts;
    {
        // user's own recent stated facts (verified)
        QList<Fact> uf = m_ctx->factsBySource(FactSource::UserMessage);
        int n = qMin(uf.size(), 5);
        for (int i = 0; i < n; ++i)
            userFacts << uf.at(uf.size() - 1 - i).content;
    }

    // ---- Topic scorer: ranked candidates instead of a flat prompt ----
    QList<ContextManager::TopicScore> topics = m_ctx->rankTopics(
        unfinished, interests, recentLines, events, userFacts);
    QStringList topTopics;
    int nt = qMin(topics.size(), 3);
    for (int i = 0; i < nt; ++i)
        topTopics << QString("- %1（%2）").arg(topics.at(i).topic, topics.at(i).source);

    // system data facts for this moment (real detection only)
    QStringList sysFacts;
    if (ConfigService::instance().allowStateRead()) {
        if (!state.isEmpty() && state != "active")
            sysFacts << QString("- [system_data] 用户当前状态：%1").arg(state);
        if (!fg.isEmpty())
            sysFacts << QString("- [system_data] 用户当前前台窗口是 %1").arg(fg);
    }

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        QString raw = watcher->result();
        // ResponseValidator gate for proactive messages too
        ResponseValidator::Result vr = ResponseValidator::validate(raw, QString());
        QString text = vr.text;
        // AI may decide to stay quiet (e.g. user is coding) -> skip
        if (text.isEmpty()) {
            watcher->deleteLater();
            return;
        }
        m_lastAiReply = text;
        // both: show in chat, and flag as a proactive (idle) message
        emit chatReply(text);
        emit idleReply(text);
        watcher->deleteLater();
    });
    // collect the process snapshot in the worker thread (one tasklist call,
    // only on idle trigger — zero cost during normal use)
    QFuture<QString> future = QtConcurrent::run(
        [mem, user, ai, fg, recent, state, aiState, topTopics, sysFacts, this]() {
        QString prompt = "你是" + ai + "，人设：" + aiPersonality() + "。用户" + user + "已经有一会儿没操作电脑了。\n"
            + "【我们的关系】" + relationshipText() + "\n"
            + "【已验证事实】（仅系统真实检测）\n"
            + (sysFacts.isEmpty() ? QString("- （用户关闭了状态感知，或当前无特殊状态）") : sysFacts.join("\n")) + "\n"
            + "【推荐话题】（按评分排序，选分数最高且合适的一个自然提起）\n"
            + (topTopics.isEmpty() ? QString("- 普通陪伴（不用涉及具体事情）") : topTopics.join("\n")) + "\n"
            + "【会话纪律】只可说上面[已验证事实]里的内容；没有任何数据时禁止'你又在打游戏''你是不是在玩'这类猜测。"
              "宁可聊推荐话题，也不猜测用户行为。\n"
            + "【当前会话状态】" + conversationStateBlock() + "\n"
            + "如果存在未解决的情绪事件，优先延续情绪话题，禁止切换成喝水/睡觉/健康提醒。\n"
            + "【减少固定模板】禁止'喝水/休息/吃饭'式查岗关心，按推荐话题自然延续。\n"
            + "【回复格式】只输出对话内容，禁止（动作）/*动作*//旁白，25字以内一句话。\n"
            + "你的当前状态：" + aiState + "\n你可以参考记忆：\n" + mem
            + "\n只输出这句话本身，若决定不打扰则输出空。";
        return callDeepSeekStatic("你是温柔可爱的AI陪伴者。", prompt);
    });
    watcher->setFuture(future);
}

// ---- short-term conversation state (never persisted) ----
QString AiService::conversationStateBlock() const
{
    return m_convo->summary().isEmpty() ? QString("（新会话，无特殊状态）") : m_convo->summary();
}

void AiService::updateConversationState(const QString &userText, const QString &aiReply, const QString &emotion)
{
    const QString t = userText.trimmed();
    if (t.isEmpty()) return;

    // 1) topic: derive from the user message (short, meaningful)
    {
        QString topic = t.left(24);
        topic.remove(QRegularExpression("^(为什么|请问|那个|嗯|啊|哦|ovo|Ovo|ovo )"));
        m_convo->topic = topic;
    }

    // 2) user emotion (strongest signal wins; reuse the text-based detector)
    m_convo->userEmotion = emotion.isEmpty() ? inferUserEmotion(t) : emotion;

    // 3) unresolved emotional issue detection — these MUST NOT be overridden
    //    by generic reminders in the next turn
    static const struct { const char *kw; const char *issue; } issues[] = {
        { "为什么", "用户表达了不被理解的委屈" },
        { "不问我", "用户感到被忽视，需要确认关系" },
        { "不重要", "用户怀疑自己是否重要，需要关系确认" },
        { "是不是我", "用户自我怀疑" },
        { "委屈", "用户感到委屈" },
        { "难过", "用户情绪低落" },
        { "伤心", "用户伤心" },
        { "孤独", "用户感到孤独" },
        { "寂寞", "用户感到寂寞" },
        { "压力", "用户压力大" },
        { "焦虑", "用户焦虑" },
        { "崩溃", "用户情绪崩溃" },
        { "想哭", "用户想哭" },
        { "想被陪伴", "用户想被陪伴" },
        { "陪陪我", "用户寻求陪伴" },
        { "抱抱", "用户寻求安慰" },
    };
    bool emotionIssue = false;
    for (const auto &it : issues) {
        if (t.contains(it.kw)) {
            m_convo->unresolvedIssue = QString::fromUtf8(it.issue);
            emotionIssue = true;
            break;
        }
    }
    // 4) resolve: user signals they are OK now
    static const QStringList resolveKws = { "没事了", "好了", "我没事", "想开了", "不难过", "笑了", "哈哈" };
    for (const QString &kw : resolveKws) {
        if (t.contains(kw)) {
            m_convo->unresolvedIssue.clear();
            emotionIssue = false;
            break;
        }
    }
    // 5) relationship mode + intent
    if (emotionIssue || !m_convo->unresolvedIssue.isEmpty()) {
        m_convo->relationshipMode = "comfort";
        m_convo->assistantIntent = "安慰并确认关系，先接住情绪";
    } else if (m_convo->userEmotion == "happy") {
        m_convo->relationshipMode = "casual";
        m_convo->assistantIntent = "陪ta开心，延续话题";
    } else if (m_convo->userEmotion == "tired") {
        m_convo->relationshipMode = "guidance";
        m_convo->assistantIntent = "温和关心，简短不啰嗦";
    } else {
        m_convo->relationshipMode = "neutral";
        m_convo->assistantIntent = "自然延续对话";
    }
    m_convo->lastUpdate = QDateTime::currentDateTime();
}

// ---- companion context API (facts / corrections / topics) ----
void AiService::notifyUserCorrection(){
    // UI hint hook: latest AI claim was wrong — reject the most recent
    // user-behavior fact so it never resurfaces as truth
    QList<Fact> uf = m_ctx->factsBySource(FactSource::UserMessage);
    for (int i = uf.size() - 1; i >= 0; --i) {
        if (!uf.at(i).rejected) {
            m_ctx->rejectFact(uf.at(i).id);
            break;
        }
    }
}

int AiService::correctionCount()
{
    return m_ctx->totalCorrections();
}

QStringList AiService::rankedTopics(int max)
{
    QStringList unfinished = unfinishedTopicsText(5).split('\n', Qt::SkipEmptyParts);
    QStringList interests = interestsText(5).split('\n', Qt::SkipEmptyParts);
    QStringList events = eventMemoryText(4).split('\n', Qt::SkipEmptyParts);
    QStringList recentLines = m_chatBuffer;
    QStringList userFacts;
    QList<Fact> uf = m_ctx->factsBySource(FactSource::UserMessage);
    int n = qMin(uf.size(), 5);
    for (int i = 0; i < n; ++i)
        userFacts << uf.at(uf.size() - 1 - i).content;

    QList<ContextManager::TopicScore> topics = m_ctx->rankTopics(
        unfinished, interests, recentLines, events, userFacts);
    QStringList out;
    int k = qMin(topics.size(), max);
    for (int i = 0; i < k; ++i)
        out << QString("%1 | score=%2 | %3").arg(topics.at(i).topic)
                                              .arg(topics.at(i).score, 0, 'f', 2)
                                              .arg(topics.at(i).source);
    return out;
}

// ---- user profile passthrough ----
QString AiService::userName() { return ConfigService::instance().userName(); }
QString AiService::avatarChar() { return ConfigService::instance().avatarChar(); }
void AiService::setUserName(const QString &v) { ConfigService::instance().setUserName(v); emit profileChanged(); }
void AiService::setAvatarChar(const QString &v) { ConfigService::instance().setAvatarChar(v); emit profileChanged(); }

// current contact (AI) profile — delegated to ContactService
QString AiService::aiName() { return ContactService::instance().currentName(); }
QString AiService::aiPersonality() { return ContactService::instance().currentPersonality(); }
void AiService::setAiName(const QString &v) { ContactService::instance().setCurrentName(v); emit profileChanged(); }
void AiService::setAiPersonality(const QString &v) { ContactService::instance().setCurrentPersonality(v); emit profileChanged(); }
QString AiService::setAiAvatar(const QString &srcPath)
{
    QString p = ContactService::instance().setCurrentAvatar(srcPath);
    if (!p.isEmpty()) emit profileChanged();
    return p;
}
QString AiService::aiAvatarPath() { return ContactService::instance().currentAvatarPath(); }

QString AiService::apiBaseUrl() { return ConfigService::instance().baseUrl(); }
QString AiService::apiModel() { return ConfigService::instance().model(); }
QString AiService::apiKey() { return ConfigService::instance().apiKey(); }
void AiService::setApiBaseUrl(const QString &v) { ConfigService::instance().setBaseUrl(v); }
void AiService::setApiModel(const QString &v) { ConfigService::instance().setModel(v); }
void AiService::setApiKey(const QString &v) { ConfigService::instance().setApiKey(v); }

static QString copyAvatar(const QString &src, const QString &name)
{
    if (src.isEmpty() || !QFile::exists(src)) return QString();
    QString dir = ConfigService::instance().configDir();
    QDir().mkpath(dir);
    QString dest = dir + "/" + name;

    // load, center-square crop, circular mask, save as PNG
    QImage img(src);
    if (img.isNull()) return QString();
    img = img.convertToFormat(QImage::Format_ARGB32);
    int side = qMin(img.width(), img.height());
    QRect crop((img.width() - side) / 2, (img.height() - side) / 2, side, side);
    QImage sq = img.copy(crop);

    // circular mask
    QImage out(side, side, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    {
        QPainter p(&out);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, side, side);
        p.setClipPath(path);
        p.drawImage(0, 0, sq);
        p.end();
    }
    QFile::remove(dest);
    if (out.save(dest, "PNG")) return dest;
    return QString();
}

QString AiService::setUserAvatar(const QString &srcPath)
{
    QString p = copyAvatar(srcPath, "user_avatar.png");
    if (!p.isEmpty()) ConfigService::instance().setAvatarChar(""); // use image over char
    return p;
}
QString AiService::userAvatarPath()
{
    QString p = ConfigService::instance().configDir() + "/user_avatar.png";
    return QFile::exists(p) ? p : QString();
}

// ---- AI-generated greeting (DeepSeek) ----
void AiService::generateGreeting()
{
    QString mem = readMemory();
    QString up = uptimeText();
    QString user = ConfigService::instance().userName();
    QString ai = ContactService::instance().currentName();
    QString personality = ContactService::instance().currentPersonality();
    int hour = QTime::currentTime().hour();
    QString period = hour < 11 ? "早上" : hour < 14 ? "中午" : hour < 18 ? "下午" : hour < 23 ? "晚上" : "深夜";

    QString prompt = "你是" + ai + "，性格" + personality + "。用户叫" + user + "。"
                     "现在是" + period + "。请用1-3句话自然地问候用户，要像朋友一样，带一点可爱。"
                     "你可以参考这些记忆，但不要生硬罗列：\n" + mem + "\n" + up
                     + "\n只输出问候语本身，不要任何前缀。";

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        ResponseValidator::Result vr = ResponseValidator::validate(watcher->result(), QString());
        emit greetingReady(vr.text);
        watcher->deleteLater();
    });
    QFuture<QString> future = QtConcurrent::run([prompt]() {
        return callDeepSeekStatic("你是温柔可爱的AI陪伴者。", prompt);
    });
    watcher->setFuture(future);
}

// ---- memory management (AIRI-style) ----
QString AiService::memoryDetail()
{
    QJsonDocument d = QJsonDocument::fromJson(readMemory().toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QStringList lines;

    QString created = o.value("created").toString();
    if (!created.isEmpty())
        lines << "首次使用：" + created.replace('T', ' ').left(16);

    QString lastDate = o.value("lastDate").toString();
    if (!lastDate.isEmpty())
        lines << "最近使用：" + lastDate;

    // notes the user told the AI to remember
    QJsonArray notes = o.value("notes").toArray();
    if (!notes.isEmpty()) {
        lines << "";
        lines << "【用户笔记】";
        for (const QJsonValue &v : notes)
            lines << "  · " + v.toString();
    }

    // recent chat topics (last few chat texts)
    QJsonArray chats = o.value("chatLog").toArray();
    if (!chats.isEmpty()) {
        lines << "";
        lines << "【最近聊天】";
        int from = qMax(0, chats.size() - 5);
        for (int i = from; i < chats.size(); i++) {
            QString c = chats.at(i).toString();
            lines << "  · " + c.left(40) + (c.size() > 40 ? "…" : "");
        }
    }

    QString sleep = o.value("sleepHabit").toString();
    if (!sleep.isEmpty()) {
        lines << "";
        lines << "睡眠习惯：" + sleep + "睡觉";
    }

    // usage minutes for recent days
    lines << "";
    lines << "每日使用时长：";
    int shown = 0;
    for (int i = 6; i >= 0; i--) {
        QString day = QDate::currentDate().addDays(-i).toString("yyyy-MM-dd");
        QString key = "usageMinutes_" + day;
        int mins = o.value(key).toInt();
        if (mins > 0) {
            int h = mins / 60, m = mins % 60;
            lines << (h > 0 ? QString("  %1：%2 小时 %3 分钟").arg(day).arg(h).arg(m)
                            : QString("  %1：%2 分钟").arg(day).arg(m));
            shown++;
        }
    }
    if (shown == 0)
        lines << "  （暂无时长记录）";

    // ---- companion: event memory ----
    QString events = eventMemoryText(8);
    if (!events.isEmpty()) {
        lines << "";
        lines << "【共同经历（事件记忆）】";
        lines << "  " + events.replace('\n', "\n  ");
    }

    // ---- relationship ----
    lines << "";
    lines << "【与用户的关系】";
    lines << "  " + relationshipText();

    // ---- AI state ----
    QJsonObject ast = readAiState();
    lines << "";
    lines << "【AI 状态】心情：" + ast.value("mood").toString("calm")
             + "，精力：" + QString::number(ast.value("energy").toInt(80));

    // ---- interests ----
    QString interests = interestsText(8);
    if (!interests.isEmpty()) {
        lines << "";
        lines << "【用户兴趣】";
        lines << "  " + interests.replace('\n', "\n  ");
    }

    // ---- unfinished topics ----
    QString topics = unfinishedTopicsText(5);
    if (!topics.isEmpty()) {
        lines << "";
        lines << "【未完成话题】";
        lines << "  " + topics.replace('\n', "\n  ");
    }

    return lines.join("\n");
}

void AiService::addMemoryNote(const QString &note)
{
    QString n = note.trimmed();
    if (n.isEmpty()) return;
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QJsonArray notes = o.value("notes").toArray();
    if (notes.size() >= 100) { // bound
        QJsonArray kept;
        for (int i = notes.size() - 99; i < notes.size(); i++) kept.append(notes.at(i));
        notes = kept;
    }
    notes.append(n + "（" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "）");
    o.insert("notes", notes);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
}

// raw memory.json access for manual editing
QString AiService::memoryRaw()
{
    return readMemory();
}

void AiService::setMemoryRaw(const QString &json)
{
    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError || !d.isObject()) return; // keep old on invalid
    QJsonObject o = d.object();
    // always preserve the first-use stamp
    QJsonObject old = QJsonDocument::fromJson(readMemory().toUtf8()).object();
    if (!o.contains("created") && old.contains("created"))
        o.insert("created", old.value("created"));
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
    m_chatBuffer.clear();
    m_userTurns = 0;
}

void AiService::clearMemory()
{
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QString created = o.value("created").toString(); // keep first-use stamp
    o = QJsonObject();
    if (!created.isEmpty()) o.insert("created", created);
    o.insert("clearedAt", QDateTime::currentDateTime().toString(Qt::ISODate));
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
    m_chatBuffer.clear();
    m_userTurns = 0;
}

// ---- auto memory: after 3+ user turns, condense the recent chat into a short note ----
void AiService::trackChatTurn(const QString &userText, const QString &aiReply)
{
    if (userText.isEmpty()) return;
    // keep the last 12 turns in the buffer (short texts only)
    QString us = userText.left(200);
    QString as = aiReply.left(200);
    QString uLine = "用户: " + us;
    m_chatBuffer.append(uLine);
    if (!as.isEmpty()) {
        QString aLine = aiName() + ": " + as;
        m_chatBuffer.append(aLine);
    }
    // trim from the front, but ALWAYS keep at least the current pair
    while (m_chatBuffer.size() > 8)
        m_chatBuffer.removeFirst();
    maybeSummarize();
}

void AiService::maybeSummarize()
{
    // summarize after 3 user turns since the last summary
    if (m_userTurns < 3 || m_summarizing || m_chatBuffer.isEmpty()) return;
    m_summarizing = true;

    QString chat = m_chatBuffer.join("\n");
    QString prompt = "下面是用户与AI的一段对话：\n\n" + chat
        + "\n\n请用最多 2 句话，简短提炼对话中值得长期记住的关于用户的信息"
          "（喜好、习惯、重要事件、情绪、近况等）。如果没什么值得记的，只输出「无」。"
          "只输出提炼内容本身，不要解释。";
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        QString note = watcher->result().trimmed();
        watcher->deleteLater();
        m_summarizing = false;
        if (note.isEmpty() || note == "无") return;
        // keep a mid-term topic summary for context continuity
        if (m_convo) m_convo->topicSummary = note;
        appendNote(note);
    });
    QFuture<QString> future = QtConcurrent::run([prompt]() {
        return callDeepSeekStatic("你是记忆整理助手，只做简洁提炼。", prompt);
    });
    watcher->setFuture(future);
}

void AiService::appendNote(const QString &note)
{
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QJsonArray notes = o.value("notes").toArray();
    if (notes.size() >= 100) { // bound
        QJsonArray kept;
        for (int i = notes.size() - 99; i < notes.size(); i++) kept.append(notes.at(i));
        notes = kept;
    }
    notes.append(note + "（" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "）");
    o.insert("notes", notes);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
    m_userTurns = 0;      // restart the counting window
    // NOTE: do NOT clear m_chatBuffer here — that wiped the current
    // conversation every ~3 turns (auto-summary), causing "short-term amnesia".
    // The recent-chat buffer must survive summarization so the AI keeps the
    // ongoing topic and emotional state.
}

// ---- seed recent-chat context from the UI (SQLite history on page open) ----
void AiService::setChatHistory(const QString &history)
{
    // NOTE: no dedupe — dropping repeated lines breaks user/AI pairing order
    // (a repeated "早点休息" would leave an AI line orphaned). Keep the last N.
    m_chatBuffer.clear();
    const QStringList lines = history.split('\n', Qt::SkipEmptyParts);
    for (const QString &l : lines) {
        QString t = l.trimmed();
        if (t.isEmpty()) continue;
        m_chatBuffer.append(t);
    }
    while (m_chatBuffer.size() > 8)
        m_chatBuffer.removeFirst();
}

// ---- DeepSeek chat: ContextManager -> FactFilter -> Prompt Builder -> LLM -> Validator ----
void AiService::sendMessage(const QString &text)
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ContactService::instance().currentName();

    // remember this user turn for auto memory (after N turns we summarize)
    QString userForMemory = text.trimmed();
    m_userTurns++;

    // ---- Context Manager: ingest user message as a verified fact ----
    m_ctx->addUserMessage(text, {"user_said"});

    // ---- correction detection (semantic): user negates the last AI claim ----
    QStringList corrected = m_ctx->detectCorrection(text, m_lastAiReply);
    for (const QString &id : corrected)
        m_ctx->rejectFact(id);

    // ---- Context Manager: ingest real system data as verified facts ----
    if (ConfigService::instance().allowStateRead()) {
        QString state = analyzeUserState();
        if (!state.isEmpty())
            m_ctx->addSystemData(state, 0.95, {"state"});
        QString fg = foregroundApp();
        if (!fg.isEmpty())
            m_ctx->addSystemData("用户当前前台窗口是 " + fg, 0.90, {"foreground"});
        QString up = uptimeText();
        if (!up.isEmpty())
            m_ctx->addSystemData(up, 0.98, {"system"});
    } else {
        m_ctx->addSystemData(QString("（用户关闭了状态感知）%1点").arg(QTime::currentTime().hour()),
                             0.95, {"state"});
    }

    // ---- Context Manager: ingest long-term memory as memory facts ----
    // load ALL notes + events (not a fixed count); MemoryRetriever will score
    // and select the most relevant ones for the current context.
    {
        QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
        QJsonObject o = d.isObject() ? d.object() : QJsonObject();
        QJsonArray notes = o.value("notes").toArray();
        int n = qMin(notes.size(), 40); // bound the session cache, not the recall
        for (int i = 0; i < n; ++i) {
            QString note = notes.at(i).toString();
            MemoryKind k = MemoryRetriever::classify(note);
            m_ctx->addMemoryFact(note, k, {"memory_note"});
        }
        QJsonArray evts = o.value("events").toArray();
        int e = qMin(evts.size(), 20);
        for (int i = 0; i < e; ++i) {
            QJsonObject ev = evts.at(i).toObject();
            QString summary = ev.value("summary").toString().trimmed();
            QString date = ev.value("date").toString();
            if (summary.isEmpty()) continue;
            Fact evFact;
            // stable unique id from content+date so dedupe/reject work reliably
            evFact.id = "mem_evt_" + QString::number(qHash(summary + "|" + date));
            evFact.content = date.isEmpty() ? summary : summary + "（" + date + "）";
            evFact.source = FactSource::Memory;
            evFact.memKind = MemoryKind::MemoryEvent;
            evFact.confidence = 0.85;
            evFact.tags = { "memory_event" };
            QDateTime dt = QDateTime::fromString(date, "yyyy-MM-dd");
            if (dt.isValid()) evFact.timestamp = dt;
            m_ctx->addFact(evFact);
        }
    }

    // companion: auto-capture emotionally significant moments as event memory
    {
        static const struct { const char *kw; const char *type; const char *label; } moods[] = {
            { "累了", "mood", "用户感到疲惫" },
            { "好累", "mood", "用户感到疲惫" },
            { "难过", "mood", "用户情绪低落" },
            { "压力", "mood", "用户压力大" },
            { "孤独", "mood", "用户感到孤独" },
            { "想被陪伴", "mood", "用户想被陪伴" },
            { "开心", "mood", "用户心情不错" },
            { "今天好", "mood", "用户心情不错" },
        };
        for (const auto &m : moods) {
            if (text.contains(m.kw)) {
                recordEvent(m.type, QString("%1（%2）").arg(m.label).arg(text.left(40)));
                break;
            }
        }
    }

    // ---- Prompt Builder: separate sections ----
    // 1) persona (tone only — never creates facts)
    // 2) verified facts (FactFilter output, code-enforced)
    // 3) hypotheses (inference, question-form only)
    // 4) user state (system_data) and AI state (mood/energy) kept apart
    QString emotion = inferUserEmotion(text);
    QString strategy;
    if (emotion == "tired")
        strategy = "用户现在比较疲惫，语气温柔体贴，少提问题多安慰，可以建议休息，别啰嗦。";
    else if (emotion == "stressed")
        strategy = "用户现在压力大，降低问题分析，多陪伴多安慰，别给建议堆砌，先接住情绪。";
    else if (emotion == "lonely")
        strategy = "用户感到孤独想被陪伴，回复温暖亲近，让ta感觉被陪着，多用关心。";
    else if (emotion == "sad")
        strategy = "用户难过，先共情安慰，再轻轻问一句怎么了，不要急着解决。";
    else if (emotion == "happy")
        strategy = "用户心情不错，陪ta开心，可以一起聊好玩的事。";
    else
        strategy = "正常聊天，自然延续。";

    // FactFilter: code-level guarantee of what can be stated as fact.
    // Score memories against the current message + conversation topic.
    QString factsText = m_ctx->factsSection(12, text, m_convo->topic);
    // debug log: how memories were recalled for this message
    qWarning("%s", qUtf8Printable(m_ctx->recallReport(text, m_convo->topic)));
    if (factsText.isEmpty())
        factsText = "（暂无已验证信息，宁可少说不可编造）";
    QString hypsText = m_ctx->hypothesesSection(3);

    QString system = "你是" + ai + "，用户叫" + user + "。\n"
        + "【人设】（只影响你的语气和表达方式，不影响你对事实的判断）\n" + aiPersonality() + "\n"
        + "【我们的关系】" + relationshipText() + "\n"
        + "规则：像真人聊天，不要客服语气，不频繁提醒自己是AI，根据用户情绪回应。\n"
        + "重要：不要机械复述或回显用户的原话，不要反复引用同一句话。"
          "在回应中自然承接上一句，但补充新角度、新细节、新问题，让对话自然延续而非原地打转。\n"
        + "【已验证事实】（只有这里列出的才可以说成确定的事实；除此之外一律不得断言用户行为）\n"
        + factsText + "\n"
        + "【推测区】（以下只是猜测，最多用'是不是/感觉/我猜'问一句，绝不能当成事实陈述）\n"
        + (hypsText.isEmpty() ? QString("（无）") : hypsText) + "\n"
        + "【事实纪律——代码强制，不可违反】\n"
          "①只允许陈述【已验证事实】区的内容，禁止编造用户行为；\n"
          "②禁止'我看到你''你刚刚一直''你又在''你是不是在'这类无据观察；\n"
          "③如果用户刚刚纠正过你（'没有''不是'），先承认错误，不要再追问相同内容；\n"
          "④宁可少说，不可编造。不确定就自然地问，或聊共同话题。\n"
        + "【角色边界】你是现实陪伴型AI，不是小说角色。禁止描述你做不到的动作（如'我去调整'），"
          "禁止虚构共同经历。陪伴说'我陪着你'，不说'我知道你发生了什么'。\n"
        + "【回复长度】普通聊天不超过25字，可拆成多条短句（换行分隔）；深入话题不超过80字。"
          "禁止长篇大论。允许轻微玩笑、撒娇、小情绪。\n"
        + "【回复格式】只输出对话内容本身。禁止括号动作（如（温柔地看着你））、星号动作（如*抱住你*）、"
          "旁白。把情绪融入对话（'没关系啦'而非（语气轻）'没关系。'）。\n"
        + "【减少固定模板】禁止频繁'喝水/休息/吃饭/打游戏'式查岗关心，多聊话题、共同经历、兴趣。\n"
        + "【话题摘要】（中期记忆：如果之前聊过一个话题并总结过，这里给出概要，帮助你延续而不失忆）\n"
        + (m_convo->topicSummary.isEmpty() ? QString("（暂无，继续当前对话）") : m_convo->topicSummary) + "\n"
        + "【当前会话状态】（保持连续性：继续当前话题和情绪，不要跳回通用提醒）\n"
        + conversationStateBlock() + "\n"
        + "【会话纪律】如果存在未解决的情绪事件（委屈/压力/孤独/寻求陪伴），必须优先处理情绪，"
          "禁止切换到睡觉/喝水/健康等普通提醒。持续回应当前话题，不要失忆式跳转。\n"
        + "当前策略：" + strategy + "\n"
        + "情绪表达：回复开头用一个情绪令牌表示你此刻的情绪，例如 <|ACT {\"emotion\":\"happy\"}|>；"
          "可用：happy, sad, angry, think, surprised, awkward, question, curious, neutral。令牌不显示给用户。\n"
        + "【我此刻的状态（AI 自身状态，与用户事实无关）】" + aiStateJson();

    // recent conversation history so the AI can see what was said before
    QString historyBlock;
    if (!m_chatBuffer.isEmpty())
        historyBlock = "\n[Recent chat]\n" + m_chatBuffer.join("\n");

    // time prefix on the user message (KV-cache friendly)
    QString userMsg = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "] "
                      + text + "\n" + historyBlock;

    // Build role-based messages so the model sees who said what: system persona,
    // then the recent chat split into user/assistant roles, then this turn.
    QJsonArray msgs;
    QJsonObject sysMsg;
    sysMsg.insert("role", "system");
    sysMsg.insert("content", system);
    msgs.append(sysMsg);
    const QString aiPrefix = ai + ": ";
    for (const QString &line : m_chatBuffer) {
        QJsonObject m;
        if (line.startsWith("用户: "))
            m.insert("role", "user");
        else if (line.startsWith(aiPrefix))
            m.insert("role", "assistant");
        else
            continue;
        // strip the role prefix, keep the raw content
        QString content = line;
        if (content.startsWith("用户: ")) content = content.mid(4);
        else if (content.startsWith(aiPrefix)) content = content.mid(aiPrefix.length());
        m.insert("content", content.trimmed());
        msgs.append(m);
    }
    QJsonObject curMsg;
    curMsg.insert("role", "user");
    curMsg.insert("content", userMsg);
    msgs.append(curMsg);

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, userForMemory, emotion, factsText]() {
        QString raw = watcher->result();

        // ---- ResponseValidator: mandatory output gate ----
        ResponseValidator::Result vr = ResponseValidator::validate(raw, factsText);
        QString speech = vr.text;

        // replay emotion tokens with small delays so the UI shows emotion changes
        // (parse them before validation strips them)
        static const QRegularExpression actRe("<\\|\\s*ACT\\s*\\{(.*?)\\}\\s*\\|>",
                                              QRegularExpression::DotMatchesEverythingOption);
        static const QRegularExpression delayRe("<\\|\\s*DELAY\\s+([0-9.]+)\\s*\\|>",
                                                QRegularExpression::CaseInsensitiveOption);
        struct Step { QString emotion; qreal intensity; int delayMs; int pos; };
        QList<Step> steps;
        auto actIt = actRe.globalMatch(raw);
        while (actIt.hasNext()) {
            auto m = actIt.next();
            QString json = m.captured(1);
            QRegularExpression emRe("\"emotion\"\\s*:\\s*\"([a-zA-Z]+)\"");
            QRegularExpression intRe("\"intensity\"\\s*:\\s*([0-9.]+)");
            auto em = emRe.match(json);
            QString e = em.hasMatch() ? em.captured(1).toLower() : QString();
            auto it = intRe.match(json);
            qreal iv = it.hasMatch() ? it.captured(1).toDouble() : 1.0;
            if (!e.isEmpty())
                steps.append({ e, qBound(0.0, iv, 1.0), 0, (int)m.capturedStart() });
        }
        auto dIt = delayRe.globalMatch(raw);
        while (dIt.hasNext()) {
            auto m = dIt.next();
            steps.append({ QString(), -1, (int)(m.captured(1).toDouble() * 1000), (int)m.capturedStart() });
        }
        std::sort(steps.begin(), steps.end(), [](const Step &a, const Step &b) { return a.pos < b.pos; });

        int running = 0;
        for (const Step &s : steps) {
            if (!s.emotion.isEmpty()) {
                running++;
                QTimer::singleShot(running * 500, this, [this, s]() { emit emotionSignal(s.emotion, s.intensity); });
            } else if (s.delayMs > 0) {
                running++;
            }
        }

        if (speech.isEmpty())
            speech = raw.isEmpty() ? "（我没想好说什么…）" : raw;
        m_lastAiReply = speech;
        // keep the conversation state in sync for the NEXT turn
        updateConversationState(userForMemory, speech, emotion);
        emit chatReply(speech);
        watcher->deleteLater();

        // auto memory: track this turn, summarize after enough turns
        trackChatTurn(userForMemory, speech);

        // proactive engine: learn interests from what the user keeps bringing up
        {
            static const struct { const char *kw; const char *interest; } interests[] = {
                { "minecraft", "Minecraft" }, { "我的世界", "Minecraft" },
                { "小说", "小说" }, { "写小说", "小说创作" },
                { "代码", "编程" }, { "软件", "软件开发" }, { "开发", "软件开发" },
                { "游戏", "游戏" }, { "音乐", "音乐" }, { "动漫", "动漫" },
                { "健身", "健身" }, { "学习", "学习" },
            };
            QString lower = userForMemory.toLower();
            for (const auto &in : interests) {
                if (lower.contains(in.kw)) { recordInterest(in.interest); break; }
            }
            // energy slowly drains per exchange, mood drifts toward user emotion
            adjustAiEnergy(-1);
            setAiMood(emotion == "happy" ? "cheerful" : emotion == "tired" ? "gentle" : "calm");
            // relationship continuity: each meaningful chat deepens it a little
            bumpRelationship(1, 0);
        }
    });
    QFuture<QString> future = QtConcurrent::run([msgs]() {
        return callDeepSeekMessages(msgs);
    });
    watcher->setFuture(future);
}

// static wrapper so the lambda doesn't capture this
QString AiService::callDeepSeekStatic(const QString &system, const QString &user)
{
    QString key = ConfigService::instance().apiKey();
    if (key.isEmpty()) return "（还没配置 API Key，去设置里填一下~）";
    QString base = ConfigService::instance().baseUrl().trimmed();
    if (base.isEmpty()) base = "https://api.deepseek.com/v1";
    QString model = ConfigService::instance().model();

    QJsonObject msg1;
    msg1.insert("role", "system");
    msg1.insert("content", system);
    QJsonObject msg2;
    msg2.insert("role", "user");
    msg2.insert("content", user);
    QJsonArray arr;
    arr.append(msg1);
    arr.append(msg2);

    QJsonObject body;
    body.insert("model", model.isEmpty() ? "deepseek-chat" : model);
    body.insert("messages", arr);
    body.insert("stream", false);
    // discourage the model from repeating the same tokens/topics (anti-looping)
    body.insert("frequency_penalty", 0.3);
    body.insert("presence_penalty", 0.3);

    QNetworkAccessManager mgr;
    QNetworkRequest req;
    req.setUrl(QUrl(base + "/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    bool timedOut = false;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    // don't hang forever if the network stalls (proxy down / slow API)
    QTimer::singleShot(30000, &loop, [&loop, &timedOut]() { timedOut = true; loop.quit(); });
    loop.exec();
    if (timedOut) {
        reply->abort();
        reply->deleteLater();
        return "（请求超时了，检查一下网络或代理~）";
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    QJsonParseError pe;
    QJsonDocument resp = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError || !resp.isObject())
        return "（请求出错：无法解析回复）";
    const QJsonArray choices = resp.object().value("choices").toArray();
    if (choices.isEmpty()) return "（没有回复内容）";
    const QJsonObject first = choices.first().toObject();
    const QJsonObject message = first.value("message").toObject();
    return message.value("content").toString("（空回复）").trimmed();
}

// ---- DeepSeek chat with a full role-based messages array ----
QString AiService::callDeepSeekMessages(const QJsonArray &messages)
{
    QString key = ConfigService::instance().apiKey();
    if (key.isEmpty()) return "（还没配置 API Key，去设置里填一下~）";
    QString base = ConfigService::instance().baseUrl().trimmed();
    if (base.isEmpty()) base = "https://api.deepseek.com/v1";
    QString model = ConfigService::instance().model();

    QJsonObject body;
    body.insert("model", model.isEmpty() ? "deepseek-chat" : model);
    body.insert("messages", messages);
    body.insert("stream", false);
    // discourage repetition / looping over the same words or topics
    body.insert("frequency_penalty", 0.3);
    body.insert("presence_penalty", 0.3);

    QNetworkAccessManager mgr;
    QNetworkRequest req;
    req.setUrl(QUrl(base + "/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    bool timedOut = false;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(30000, &loop, [&loop, &timedOut]() { timedOut = true; loop.quit(); });
    loop.exec();
    if (timedOut) {
        reply->abort();
        reply->deleteLater();
        return "（请求超时了，检查一下网络或代理~）";
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    QJsonParseError pe;
    QJsonDocument resp = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError || !resp.isObject())
        return "（请求出错：无法解析回复）";
    const QJsonArray choices = resp.object().value("choices").toArray();
    if (choices.isEmpty()) return "（没有回复内容）";
    const QJsonObject first = choices.first().toObject();
    const QJsonObject message = first.value("message").toObject();
    return message.value("content").toString("（空回复）").trimmed();
}

// ================= Batch 1: companion relationship + event memory =================

// ---- record an event memory with date, e.g. recordEvent("mood", "晚上独处，感觉疲惫，想被陪伴") ----
void AiService::recordEvent(const QString &type, const QString &summary)
{
    if (!ConfigService::instance().allowLongTermMemory()) return;
    QString s = summary.trimmed();
    if (s.isEmpty()) return;
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QJsonArray events = o.value("events").toArray();
    QJsonObject ev;
    ev.insert("date", QDate::currentDate().toString("yyyy-MM-dd"));
    ev.insert("time", QDateTime::currentDateTime().toString("HH:mm"));
    ev.insert("type", type);
    ev.insert("summary", s);
    events.append(ev);
    if (events.size() > 50) {
        QJsonArray kept;
        for (int i = events.size() - 50; i < events.size(); i++) kept.append(events.at(i));
        events = kept;
    }
    o.insert("events", events);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
}

// ---- recent event memories for prompt injection ----
QString AiService::eventMemoryText(int maxEvents)
{
    if (!ConfigService::instance().allowLongTermMemory()) return QString();
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    if (!d.isObject()) return QString();
    QJsonArray events = d.object().value("events").toArray();
    if (events.isEmpty()) return QString();
    QStringList lines;
    int from = qMax(0, events.size() - maxEvents);
    for (int i = from; i < events.size(); i++) {
        QJsonObject ev = events.at(i).toObject();
        lines << "- [" + ev.value("date").toString() + " " + ev.value("time").toString() + "] "
                 + ev.value("summary").toString();
    }
    return lines.join("\n");
}

// ================= Batch 2: system state sensing + analysis =================

#ifdef Q_OS_WIN
// helper: absolute diff between two FILETIMEs
static ULONGLONG filetimeDelta(const FILETIME &a, const FILETIME &b)
{
    ULARGE_INTEGER x, y;
    x.LowPart = a.dwLowDateTime; x.HighPart = a.dwHighDateTime;
    y.LowPart = b.dwLowDateTime; y.HighPart = b.dwHighDateTime;
    return x.QuadPart > y.QuadPart ? x.QuadPart - y.QuadPart : 0;
}
#endif

// light-weight CPU usage (%). Uses GetSystemTimes deltas; on failure returns -1.
static int cpuPercent()
{
#ifdef Q_OS_WIN
    FILETIME id1, kr1, us1, id2, kr2, us2;
    if (!GetSystemTimes(&id1, &kr1, &us1)) return -1;
    Sleep(200);
    if (!GetSystemTimes(&id2, &kr2, &us2)) return -1;
    ULONGLONG idle = filetimeDelta(id2, id1);
    ULONGLONG kern = filetimeDelta(kr2, kr1) + filetimeDelta(us2, us1);
    if (kern == 0) return -1;
    int pct = (int)(100.0 - 100.0 * (double)idle / (double)kern);
    return qBound(0, pct, 100);
#else
    return -1;
#endif
}

// battery percent (or -1 if no battery / desktop)
static int batteryPercent()
{
#ifdef Q_OS_WIN
    SYSTEM_POWER_STATUS sps;
    if (!GetSystemPowerStatus(&sps)) return -1;
    if (sps.BatteryFlag == 128) return -1; // no battery
    return (int)sps.BatteryLifePercent;
#else
    return -1;
#endif
}

QString AiService::batteryState()
{
    int pct = batteryPercent();
    if (pct < 0) return QString(); // desktop without battery
    return QString("电量 %1%").arg(pct);
}

// raw system snapshot as JSON
QString AiService::systemStateJson()
{
    QJsonObject o;
    o.insert("cpu", cpuPercent());
    o.insert("hour", QTime::currentTime().hour());
    o.insert("minute", QTime::currentTime().minute());
    o.insert("foreground", foregroundApp());
    o.insert("idleMs", lastInputMs());
    o.insert("battery", batteryPercent());
    o.insert("uptime", uptimeText());
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

// human-readable state analysis for the AI prompt
QString AiService::analyzeUserState()
{
    if (!ConfigService::instance().allowStateRead())
        return QString("（用户关闭了状态感知，只保留基础时间信息）" +
                       QString("%1点").arg(QTime::currentTime().hour()));

    int hour = QTime::currentTime().hour();
    QString fg = foregroundApp();
    qint64 idle = lastInputMs();

    QStringList parts;

    if (hour >= 23 || hour < 5)
        parts << "深夜(用户还没睡)";
    else if (hour < 8)
        parts << "清晨";
    else if (hour < 12)
        parts << "上午";
    else if (hour < 14)
        parts << "中午";
    else if (hour < 18)
        parts << "下午";
    else
        parts << "晚上";

    if (idle >= 0 && idle >= 15 * 60 * 1000)
        parts << "已经15分钟没操作(可能离开/在看别的)";
    else if (!fg.isEmpty())
        parts << ("正在使用:" + fg);

    QString bat = batteryState();
    if (!bat.isEmpty())
        parts << bat;

    return parts.join("，");
}

// ---- batch 3: infer user emotion from text + time/context ----
QString AiService::inferUserEmotion(const QString &text)
{
    const QString t = text.trimmed();
    // text signals first (strongest)
    if (t.contains("累") || t.contains("困") || t.contains("疲惫"))
        return "tired";
    if (t.contains("烦") || t.contains("压力") || t.contains("崩溃") || t.contains("焦虑"))
        return "stressed";
    if (t.contains("孤独") || t.contains("寂寞") || t.contains("一个人") || t.contains("想被陪伴"))
        return "lonely";
    if (t.contains("开心") || t.contains("哈哈") || t.contains("太好了") || t.contains("高兴"))
        return "happy";
    if (t.contains("难过") || t.contains("伤心") || t.contains("想哭") || t.contains("失落"))
        return "sad";
    // weak signal: late night + short reply → tired
    int hour = QTime::currentTime().hour();
    if ((hour >= 23 || hour < 5) && t.length() < 20)
        return "tired";
    return "normal";
}

// ---- privacy toggles (forwarded to ConfigService) ----
bool AiService::allowStateRead() { return ConfigService::instance().allowStateRead(); }
bool AiService::allowTimeRecord() { return ConfigService::instance().allowTimeRecord(); }
bool AiService::allowLongTermMemory() { return ConfigService::instance().allowLongTermMemory(); }
void AiService::setAllowStateRead(bool v) { ConfigService::instance().setAllowStateRead(v); }
void AiService::setAllowTimeRecord(bool v) { ConfigService::instance().setAllowTimeRecord(v); }
void AiService::setAllowLongTermMemory(bool v) { ConfigService::instance().setAllowLongTermMemory(v); }

// ================= Batch A: proactive conversation engine =================

// state file: %APPDATA%/XiaoQin/XiaoQinTools/ai_state.json
static QString aiStatePath()
{
    return ConfigService::instance().configDir() + "/ai_state.json";
}

static QJsonObject readAiState()
{
    QFile f(aiStatePath());
    QJsonObject o;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) o = d.object();
        f.close();
    }
    if (o.isEmpty()) {
        o.insert("mood", "calm");
        o.insert("energy", 80);
    }
    return o;
}

static void writeAiState(const QJsonObject &o)
{
    QFile f(aiStatePath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(o).toJson());
        f.close();
    }
}

QString AiService::aiStateJson()
{
    QJsonObject o = readAiState();
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void AiService::setAiMood(const QString &mood)
{
    QJsonObject o = readAiState();
    o.insert("mood", mood);
    writeAiState(o);
}

void AiService::adjustAiEnergy(int delta)
{
    QJsonObject o = readAiState();
    int e = qBound(0, o.value("energy").toInt() + delta, 100);
    o.insert("energy", e);
    writeAiState(o);
}

int AiService::relationshipIntimacy()
{
    QJsonObject o = readRelationship();
    return o.value("intimacy").toInt();
}
int AiService::relationshipTrust()
{
    QJsonObject o = readRelationship();
    return o.value("trust").toInt();
}

// ---- friendly interest list ----
QStringList AiService::interestList()
{
    QFile f(interestPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!d.isObject()) return {};
    return d.object().keys();
}
void AiService::addInterest(const QString &name)
{
    QString n = name.trimmed();
    if (n.isEmpty()) return;
    QJsonObject o;
    QFile f(interestPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) o = d.object();
        f.close();
    }
    int cur = o.value(n).toInt();
    o.insert(n, cur + 1);
    QFile wf(interestPath());
    if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        wf.write(QJsonDocument(o).toJson());
        wf.close();
    }
}
void AiService::removeInterest(const QString &name)
{
    QJsonObject o;
    QFile f(interestPath());
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) o = d.object();
        f.close();
    }
    o.remove(name);
    QFile wf(interestPath());
    if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        wf.write(QJsonDocument(o).toJson());
        wf.close();
    }
}

// ---- friendly topic list ----
QStringList AiService::topicList()
{
    QFile f(unfinishedPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!d.isArray()) return {};
    QStringList out;
    for (const QJsonValue &v : d.array())
        out << v.toObject().value("topic").toString();
    return out;
}
void AiService::addTopic(const QString &topic)
{
    trackUnfinishedTopic(topic, 80);
}
void AiService::removeTopic(const QString &topic)
{
    QFile f(unfinishedPath());
    QJsonArray arr;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isArray()) arr = d.array();
        f.close();
    }
    QJsonArray kept;
    for (const QJsonValue &v : arr)
        if (v.toObject().value("topic").toString() != topic) kept.append(v);
    QFile wf(unfinishedPath());
    if (wf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        wf.write(QJsonDocument(kept).toJson());
        wf.close();
    }
}

// ---- friendly note list ----
QStringList AiService::noteList()
{
    QJsonDocument d = QJsonDocument::fromJson(readMemory().toUtf8());
    if (!d.isObject()) return {};
    QStringList out;
    for (const QJsonValue &v : d.object().value("notes").toArray())
        out << v.toString();
    return out;
}
void AiService::removeNote(int index)
{
    QString mem = readMemory();
    QJsonDocument d = QJsonDocument::fromJson(mem.toUtf8());
    if (!d.isObject()) return;
    QJsonObject o = d.object();
    QJsonArray notes = o.value("notes").toArray();
    if (index < 0 || index >= notes.size()) return;
    notes.removeAt(index);
    o.insert("notes", notes);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
}

// ---- unfinished topics ----
static QString unfinishedPath()
{
    return ConfigService::instance().configDir() + "/unfinished_topics.json";
}

void AiService::trackUnfinishedTopic(const QString &topic, int importance)
{
    QString t = topic.trimmed();
    if (t.isEmpty()) return;
    QFile f(unfinishedPath());
    QJsonArray arr;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isArray()) arr = d.array();
        f.close();
    }
    // remove an existing entry with the same topic, then prepend the fresh one
    QJsonArray kept;
    for (const QJsonValue &v : arr) {
        if (v.toObject().value("topic").toString() != t) kept.append(v);
    }
    QJsonObject entry;
    entry.insert("topic", t);
    entry.insert("importance", importance);
    entry.insert("last_time", QDateTime::currentDateTime().toString("MM-dd HH:mm"));
    entry.insert("status", "unfinished");
    kept.prepend(entry);
    if (kept.size() > 20) { // bound
        QJsonArray trimmed;
        for (int i = 0; i < 20; i++) trimmed.append(kept.at(i));
        kept = trimmed;
    }
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(kept).toJson());
        f.close();
    }
}

QString AiService::unfinishedTopicsText(int max)
{
    QFile f(unfinishedPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!d.isArray()) return QString();
    QJsonArray arr = d.array();
    QStringList lines;
    int n = qMin(arr.size(), max);
    for (int i = 0; i < n; i++) {
        QJsonObject o = arr.at(i).toObject();
        lines << "- [" + o.value("last_time").toString() + "] " + o.value("topic").toString();
    }
    return lines.join("\n");
}

// ---- interests (weighted topics the user cares about) ----
static QString interestPath()
{
    return ConfigService::instance().configDir() + "/interests.json";
}

void AiService::recordInterest(const QString &interest)
{
    QString in = interest.trimmed();
    if (in.isEmpty()) return;
    QFile f(interestPath());
    QJsonObject o;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) o = d.object();
        f.close();
    }
    int cur = o.value(in).toInt();
    o.insert(in, cur + 1);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(o).toJson());
        f.close();
    }
}

QString AiService::interestsText(int max)
{
    QFile f(interestPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
    QJsonDocument d = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!d.isObject()) return QString();
    QJsonObject o = d.object();
    QMultiMap<int, QString> sorted; // weight -> interest
    for (auto it = o.constBegin(); it != o.constEnd(); ++it)
        sorted.insert(it.value().toInt(), it.key());
    QStringList lines;
    QStringList keys = sorted.values();
    int n = qMin(keys.size(), max);
    for (int i = keys.size() - 1, c = 0; c < n && i >= 0; i--, c++)
        lines << "- " + keys.at(i);
    return lines.join("\n");
}

// ---- busy / activity state ----
bool AiService::isUserBusy()
{
    QString fg = foregroundApp();
    // coding / writing-heavy apps: don't interrupt
    static const QStringList busyApps = {
        "code", "visual studio", "clion", "pycharm", "idea", "xiaoqintools",
        "notepad", "word", "wps", "typora", "obsidian", "sublime", "vim",
    };
    for (const QString &b : busyApps)
        if (fg.contains(b, Qt::CaseInsensitive)) return true;
    return false;
}

QString AiService::userActivityState()
{
    int hour = QTime::currentTime().hour();
    qint64 idle = lastInputMs();
    QString fg = foregroundApp();

    if (idle >= 0 && idle >= 30 * 60 * 1000)
        return "away"; // long gone
    if (hour >= 23 || hour < 5)
        return "late_night";
    if (isForegroundMinecraft() || isGameRunning())
        return "gaming";
    if (isUserBusy())
        return "coding";
    if (idle >= 0 && idle >= 5 * 60 * 1000)
        return "idle";
    if (!fg.isEmpty())
        return "active";
    return "idle";
}

// ================= Batch B: relationship =================

// relationship continuity (relationship.json next to memory)
static QString relationshipPathB()
{
    return ContactService::instance().contactDir(ContactService::instance().currentId()) + "/relationship.json";
}

static QJsonObject readRelationship()
{
    QFile f(relationshipPathB());
    QJsonObject o;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) o = d.object();
        f.close();
    }
    if (o.isEmpty()) {
        o.insert("intimacy", 0);
        o.insert("trust", 0);
        o.insert("interaction_days", 0);
        o.insert("first_met", QDate::currentDate().toString("yyyy-MM-dd"));
    }
    return o;
}

QString AiService::relationshipText()
{
    QJsonObject o = readRelationship();
    int itm = o.value("intimacy").toInt();
    int tr = o.value("trust").toInt();
    int days = o.value("interaction_days").toInt();
    QString level;
    if (itm < 20) level = "刚认识";
    else if (itm < 40) level = "逐渐熟悉";
    else if (itm < 60) level = "好朋友";
    else if (itm < 80) level = "很亲近";
    else level = "形影不离";
    return QString("我们认识约%1天，现在是%2的关系（亲密度%3/信任度%4）。保持这个关系连续性，不要每次像第一次认识。")
        .arg(days).arg(level).arg(itm).arg(tr);
}

void AiService::bumpRelationship(int intimacyDelta, int trustDelta)
{
    QJsonObject o = readRelationship();
    int itm = qBound(0, o.value("intimacy").toInt() + intimacyDelta, 100);
    int tr = qBound(0, o.value("trust").toInt() + trustDelta, 100);
    int days = o.value("interaction_days").toInt();
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString last = o.value("last_date").toString();
    if (last != today) {
        days++;
        o.insert("last_date", today);
    }
    o.insert("intimacy", itm);
    o.insert("trust", tr);
    o.insert("interaction_days", days);
    QDir().mkpath(ContactService::instance().contactDir(ContactService::instance().currentId()));
    QFile f(relationshipPathB());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(o).toJson());
        f.close();
    }
}

// ================= Batch: editable categories for the memory UI =================

void AiService::setRelationship(int intimacy, int trust)
{
    QJsonObject o = readRelationship();
    o.insert("intimacy", qBound(0, intimacy, 100));
    o.insert("trust", qBound(0, trust, 100));
    o.insert("interaction_days", o.value("interaction_days").toInt());
    QDir().mkpath(ContactService::instance().contactDir(ContactService::instance().currentId()));
    QFile f(relationshipPathB());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(o).toJson());
        f.close();
    }
}

QString AiService::interestsRaw()
{
    QFile f(interestPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return "{}";
    QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s;
}

void AiService::setInterestsRaw(const QString &json)
{
    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError || !d.isObject()) return;
    QFile f(interestPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(d.toJson());
        f.close();
    }
}

QString AiService::unfinishedRaw()
{
    QFile f(unfinishedPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return "[]";
    QString s = QString::fromUtf8(f.readAll());
    f.close();
    return s;
}

void AiService::setUnfinishedRaw(const QString &json)
{
    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(json.toUtf8(), &pe);
    if (pe.error != QJsonParseError::NoError || !d.isArray()) return;
    QFile f(unfinishedPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(d.toJson());
        f.close();
    }
}

void AiService::setAiEnergy(int energy)
{
    QJsonObject o = readAiState();
    o.insert("energy", qBound(0, energy, 100));
    writeAiState(o);
}

// user notes only
QString AiService::notesText()
{
    QJsonDocument d = QJsonDocument::fromJson(readMemory().toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QJsonArray notes = o.value("notes").toArray();
    QStringList lines;
    if (notes.isEmpty()) return "（还没有笔记，点开聊天页可以告诉我值得记住的事）";
    for (const QJsonValue &v : notes)
        lines << "· " + v.toString();
    return lines.join("\n");
}

// usage duration only
QString AiService::usageText()
{
    QJsonDocument d = QJsonDocument::fromJson(readMemory().toUtf8());
    QJsonObject o = d.isObject() ? d.object() : QJsonObject();
    QStringList lines;
    int shown = 0;
    for (int i = 6; i >= 0; i--) {
        QString day = QDate::currentDate().addDays(-i).toString("yyyy-MM-dd");
        QString key = "usageMinutes_" + day;
        int mins = o.value(key).toInt();
        if (mins > 0) {
            int h = mins / 60, m = mins % 60;
            lines << (h > 0 ? QString("%1：%2 小时 %3 分钟").arg(day).arg(h).arg(m)
                            : QString("%1：%2 分钟").arg(day).arg(m));
            shown++;
        }
    }
    if (shown == 0) return "（暂无时长记录）";
    return lines.join("\n");
}
