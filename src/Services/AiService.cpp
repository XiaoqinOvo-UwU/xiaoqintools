#include "AiService.h"
#include "ConfigService.h"
#include "ContactService.h"

#include <QDir>
#include <QFile>
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

AiService::AiService(QObject *parent)
    : QObject(parent)
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

// ---- light-weight process snapshot (only for idle chat, runs in background) ----
static QString processSnapshot()
{
#ifdef Q_OS_WIN
    QProcess p;
    p.start("tasklist", QStringList() << "/FO" << "CSV" << "/NH");
    if (!p.waitForFinished(3000)) return QString();
    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    QStringList names;
    // parse "name.exe","pid",... lines, dedupe, cap at 30
    for (const QString &line : out.split('\n')) {
        int q1 = line.indexOf('"');
        int q2 = line.indexOf('"', q1 + 1);
        if (q1 < 0 || q2 < 0) continue;
        QString name = line.mid(q1 + 1, q2 - q1 - 1).toLower();
        if (name.isEmpty()) continue;
        if (!names.contains(name)) names.append(name);
        if (names.size() >= 30) break;
    }
    return names.join(", ");
#else
    return QString();
#endif
}

// ---- idle chat: AI proactively starts a topic ----
void AiService::idleChat()
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ContactService::instance().currentName();
    QString personality = ContactService::instance().currentPersonality();
    QString activity = activitySummary(); // already cheap (in-memory/JSON)
    QString fg = foregroundApp();         // cheap win32 call
    QString recent = m_chatBuffer.join("\n"); // last few messages if any

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
        QString raw = watcher->result();
        // strip emotion tokens too (idle chat needs no playback)
        static const QRegularExpression actRe("<\\|\\s*ACT\\s*\\{(.*?)\\}\\s*\\|>",
                                              QRegularExpression::DotMatchesEverythingOption);
        static const QRegularExpression delayRe("<\\|\\s*DELAY\\s+[0-9.]+\\s*\\|>",
                                                QRegularExpression::CaseInsensitiveOption);
        QString out = raw;
        out.remove(actRe);
        out.remove(delayRe);
        QString text = out.trimmed();
        // both: show in chat, and flag as a proactive (idle) message
        emit chatReply(text);
        emit idleReply(text);
        watcher->deleteLater();
    });
    // collect the process snapshot in the worker thread (one tasklist call,
    // only on idle trigger — zero cost during normal use)
    QFuture<QString> future = QtConcurrent::run([mem, user, ai, personality, activity, fg, recent, this]() {
        QString procs = processSnapshot();
        QStringList ctx;
        QString state = analyzeUserState();
        if (!state.isEmpty())
            ctx << "- 用户当前状态：" + state;
        if (!activity.isEmpty() && !activity.startsWith("（"))
            ctx << "- 用户今天大部分时间在：" + activity;
        if (!fg.isEmpty())
            ctx << "- 用户当前正在使用：" + fg;
        if (!procs.isEmpty())
            ctx << "- 用户电脑正在运行的进程（节选）：" + procs;
        QString recentBlock;
        if (!recent.isEmpty())
            recentBlock = "\n你们最近聊的：\n" + recent + "\n";
        QString prompt = "你是" + ai + "，性格" + personality + "。用户" + user + "已经有一会儿没操作电脑了。\n"
            + ctx.join("\n")
            + recentBlock
            + "\n请结合上面的信息和用户当前状态，自然地主动找个话题和ta聊一句。"
              "如果用户深夜还没睡，语气更温柔关心；如果用户在打游戏，就轻松地提一句；"
              "如果用户刚离开回来，就用'回来啦'打招呼。"
              "可以接着上次聊的话题，也可以聊聊ta在玩/在做的、关心的、或随便分享。"
              "像朋友一样，简短一句话，不要生硬，不要罗列数据。\n"
              "你可以参考记忆：\n" + mem + "\n只输出这句话本身。";
        return callDeepSeekStatic("你是温柔可爱的AI陪伴者。", prompt);
    });
    watcher->setFuture(future);
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
        emit greetingReady(watcher->result());
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
    notes.append(n + "（" + QDate::currentDate().toString("yyyy-MM-dd") + "）");
    o.insert("notes", notes);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
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
    // keep the last 10 turns in the buffer (short texts only)
    QString us = userText.left(200);
    QString as = aiReply.left(200);
    QString uLine = "用户: " + us;
    if (m_chatBuffer.isEmpty() || m_chatBuffer.last() != uLine)
        m_chatBuffer.append(uLine);
    if (!as.isEmpty()) {
        QString aLine = aiName() + ": " + as;
        if (m_chatBuffer.isEmpty() || m_chatBuffer.last() != aLine)
            m_chatBuffer.append(aLine);
    }
    while (m_chatBuffer.size() > 10)
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
    notes.append(note + "（" + QDate::currentDate().toString("yyyy-MM-dd") + "）");
    o.insert("notes", notes);
    writeMemory(QString::fromUtf8(QJsonDocument(o).toJson()));
    m_userTurns = 0;      // restart the counting window
    m_chatBuffer.clear(); // fresh window
}

// ---- seed recent-chat context from the UI (SQLite history on page open) ----
void AiService::setChatHistory(const QString &history)
{
    // split on newlines; each line is one message already formatted by the UI
    m_chatBuffer.clear();
    const QStringList lines = history.split('\n', Qt::SkipEmptyParts);
    for (const QString &l : lines) {
        QString t = l.trimmed();
        if (t.isEmpty()) continue;
        if (m_chatBuffer.contains(t)) continue; // dedupe — avoid repeating the same line
        m_chatBuffer.append(t);
    }
    while (m_chatBuffer.size() > 10)
        m_chatBuffer.removeFirst();
}

// ---- DeepSeek chat (AIRI-style: bucketed context + time prefix + emotion tokens) ----
void AiService::sendMessage(const QString &text)
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ContactService::instance().currentName();
    QString personality = ContactService::instance().currentPersonality();

    // remember this user turn for auto memory (after N turns we summarize)
    QString userForMemory = text.trimmed();
    m_userTurns++;

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

    // Bucket 1: persona + rules (system prompt)
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

    QString system = "你是" + ai + "，性格" + personality + "，用户叫" + user + "。\n"
        + "规则：像真人聊天，不要客服语气，不频繁提醒自己是AI，根据用户情绪回应。\n"
        + "重要：不要机械复述或回显用户的原话，不要反复引用同一句话。"
          "在回应中自然承接上一句，但补充新角度、新细节、新问题，让对话自然延续而非原地打转。"
          "如果用户重复提起同一个话题，简短回应后自然地延伸到新的相关话题。\n"
        + "当前策略：" + strategy + "\n"
        + "情绪表达：回复开头用一个情绪令牌表示你此刻的情绪，例如 <|ACT {\"emotion\":\"happy\"}|>；"
          "若中途情绪变化，在变化处再插一个令牌；需要停顿节奏时可插入 <|DELAY 1|>（数字为秒）。"
          "可用情绪：happy, sad, angry, think, surprised, awkward, question, curious, neutral。"
          "令牌不会显示给用户，不要解释它们。\n"
        // companion: event memory makes the AI a persistent partner,
        // not a fresh chatbot every session
        + "【我记得的共同经历】\n" + (eventMemoryText(5).isEmpty() ? QString("（还没有太多回忆，慢慢积累）") : eventMemoryText(5));

    // Bucket 2: memory + current state, flattened bullet list.
    // Keep memory small (last few core notes) so we don't flood the prompt and
    // cause the model to echo the user's own words back at them.
    QStringList ctx;
    if (!mem.isEmpty() && mem != "{}") {
        QStringList shortMem;
        // only take a bounded slice of memory lines to reduce noise
        const QStringList memLines = mem.split('\n', Qt::SkipEmptyParts);
        int maxLines = qMin(memLines.size(), 12);
        for (int i = 0; i < maxLines; i++)
            shortMem << memLines.at(i);
        ctx << "- memory: " + shortMem.join(" ");
    }
    ctx << "- state: " + analyzeUserState();
    QString fg = foregroundApp();
    if (!fg.isEmpty())
        ctx << "- app: 用户现在正在使用 " + fg;
    QString act = activitySummary();
    if (!act.isEmpty() && !act.startsWith("（"))
        ctx << "- activity: " + act;
    QString contextBlock = "[Context]\n" + ctx.join("\n");

    // recent conversation history so the AI can see what was said before
    QString historyBlock;
    if (!m_chatBuffer.isEmpty())
        historyBlock = "\n[Recent chat]\n" + m_chatBuffer.join("\n");

    // time prefix on the user message (KV-cache friendly)
    QString userMsg = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "] "
                      + text + "\n" + contextBlock + historyBlock;

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
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, userForMemory]() {
        QString raw = watcher->result();

        // strip <think> tags the model may add on its own
        static const QRegularExpression thinkRe("<think>(.*?)</think>", QRegularExpression::DotMatchesEverythingOption);
        QString speech = raw;
        auto tm = thinkRe.match(raw);
        if (tm.hasMatch())
            speech = raw.mid(0, tm.capturedStart()) + raw.mid(tm.capturedEnd());

        // parse AIRI-style control tokens: <|ACT {...}|> (emotion) and <|DELAY n|>
        static const QRegularExpression actRe("<\\|\\s*ACT\\s*\\{(.*?)\\}\\s*\\|>",
                                              QRegularExpression::DotMatchesEverythingOption);
        static const QRegularExpression delayRe("<\\|\\s*DELAY\\s+([0-9.]+)\\s*\\|>",
                                                QRegularExpression::CaseInsensitiveOption);
        struct Step { QString emotion; qreal intensity; int delayMs; int pos; };
        QList<Step> steps;
        auto actIt = actRe.globalMatch(speech);
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
        auto dIt = delayRe.globalMatch(speech);
        while (dIt.hasNext()) {
            auto m = dIt.next();
            steps.append({ QString(), -1, (int)(m.captured(1).toDouble() * 1000), (int)m.capturedStart() });
        }
        // sort tokens by their position in the text to keep ordering
        std::sort(steps.begin(), steps.end(), [](const Step &a, const Step &b) { return a.pos < b.pos; });
        // drop ACT/DELAY tokens from the visible text
        speech.remove(actRe);
        speech.remove(delayRe);
        speech = speech.trimmed();

        // replay emotion tokens with small delays so the UI shows emotion changes (no threads, timer-based)
        int running = 0;
        for (const Step &s : steps) {
            if (!s.emotion.isEmpty()) {
                running++;
                QTimer::singleShot(running * 500, this, [this, s]() { emit emotionSignal(s.emotion, s.intensity); });
            } else if (s.delayMs > 0) {
                running++;
            }
        }
        emit chatReply(speech.isEmpty() ? raw : speech);
        watcher->deleteLater();

        // auto memory: track this turn, summarize after enough turns
        trackChatTurn(userForMemory, speech.isEmpty() ? raw : speech);
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
