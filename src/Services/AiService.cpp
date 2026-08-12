#include "AiService.h"
#include "ConfigService.h"

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
#include <algorithm>

AiService::AiService(QObject *parent)
    : QObject(parent)
{
    ensureMemory();
}

QString AiService::memoryPath() const
{
    // memory lives in %APPDATA% so it survives deleting the tool directory
    return ConfigService::instance().configDir() + "/ai_memory.json";
}

void AiService::ensureMemory()
{
    QDir().mkpath(ConfigService::instance().configDir());

    // migrate legacy memory files if present
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
        }
    }
    mem = jsonSet(mem, "lastSessionStart", now.toString(Qt::ISODate));
    mem = jsonSet(mem, "lastDate", today);
    writeMemory(mem);
}

void AiService::recordSessionEnd()
{
    QString mem = readMemory();
    QString end = QDateTime::currentDateTime().toString(Qt::ISODate);
    mem = jsonSet(mem, "lastSessionEnd", end);

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
    // common games the user plays
    QStringList games = { "r5apex.exe", "javaw.exe", "java.exe", "Minecraft", "steam.exe" };
    QProcess p;
    p.start("tasklist", QStringList() << "/FO" << "CSV");
    if (!p.waitForFinished(1500)) return false;
    QString out = QString::fromLocal8Bit(p.readAllStandardOutput());
    for (const QString &g : games)
        if (out.contains(g, Qt::CaseInsensitive)) return true;
    return false;
}

// ---- idle chat: AI proactively starts a topic ----
void AiService::idleChat()
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ConfigService::instance().aiName();
    QString personality = ConfigService::instance().aiPersonality();
    QString prompt = "你是" + ai + "，性格" + personality + "。用户" + user + "已经有一会儿没操作电脑了。"
                     "请自然地主动找个话题和ta聊一句（关心、分享、或随便聊聊），像朋友一样，简短一句话，不要生硬。"
                     "你可以参考记忆：\n" + mem + "\n只输出这句话本身。";
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
        emit chatReply(out.trimmed());
        watcher->deleteLater();
    });
    QFuture<QString> future = QtConcurrent::run([prompt]() {
        return callDeepSeekStatic("你是温柔可爱的AI陪伴者。", prompt);
    });
    watcher->setFuture(future);
}

// ---- user profile passthrough ----
QString AiService::userName() { return ConfigService::instance().userName(); }
QString AiService::avatarChar() { return ConfigService::instance().avatarChar(); }
QString AiService::aiName() { return ConfigService::instance().aiName(); }
QString AiService::aiPersonality() { return ConfigService::instance().aiPersonality(); }
void AiService::setUserName(const QString &v) { ConfigService::instance().setUserName(v); emit profileChanged(); }
void AiService::setAvatarChar(const QString &v) { ConfigService::instance().setAvatarChar(v); emit profileChanged(); }
void AiService::setAiName(const QString &v) { ConfigService::instance().setAiName(v); emit profileChanged(); }
void AiService::setAiPersonality(const QString &v) { ConfigService::instance().setAiPersonality(v); emit profileChanged(); }

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
QString AiService::setAiAvatar(const QString &srcPath)
{
    QString p = copyAvatar(srcPath, "ai_avatar.png");
    return p;
}
QString AiService::userAvatarPath()
{
    QString p = ConfigService::instance().configDir() + "/user_avatar.png";
    return QFile::exists(p) ? p : QString();
}
QString AiService::aiAvatarPath()
{
    QString p = ConfigService::instance().configDir() + "/ai_avatar.png";
    return QFile::exists(p) ? p : QString();
}

// ---- AI-generated greeting (DeepSeek) ----
void AiService::generateGreeting()
{
    QString mem = readMemory();
    QString up = uptimeText();
    QString user = ConfigService::instance().userName();
    QString ai = ConfigService::instance().aiName();
    QString personality = ConfigService::instance().aiPersonality();
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
}

// ---- DeepSeek chat (AIRI-style: bucketed context + time prefix + emotion tokens) ----
void AiService::sendMessage(const QString &text)
{
    QString mem = readMemory();
    QString user = ConfigService::instance().userName();
    QString ai = ConfigService::instance().aiName();
    QString personality = ConfigService::instance().aiPersonality();
    int hour = QTime::currentTime().hour();
    QString period = hour < 11 ? "早上" : hour < 14 ? "中午" : hour < 18 ? "下午" : hour < 23 ? "晚上" : "深夜";

    // Bucket 1: persona + rules (system prompt)
    QString system = "你是" + ai + "，性格" + personality + "，用户叫" + user + "。\n"
        + "规则：像真人聊天，不要客服语气，不频繁提醒自己是AI，根据用户情绪回应。\n"
        + "情绪表达：回复开头用一个情绪令牌表示你此刻的情绪，例如 <|ACT {\"emotion\":\"happy\"}|>；"
          "若中途情绪变化，在变化处再插一个令牌；需要停顿节奏时可插入 <|DELAY 1|>（数字为秒）。"
          "可用情绪：happy, sad, angry, think, surprised, awkward, question, curious, neutral。"
          "令牌不会显示给用户，不要解释它们。";

    // Bucket 2: memory + current state, flattened bullet list (AIRI style, no XML so weak models don't echo it)
    QStringList ctx;
    if (!mem.isEmpty() && mem != "{}")
        ctx << "- memory: " + mem;
    ctx << "- state: 现在是" + period + "。" + uptimeText();
    QString fg = foregroundApp();
    if (!fg.isEmpty())
        ctx << "- app: 用户现在正在使用 " + fg;
    QString contextBlock = "[Context]\n" + ctx.join("\n");

    // time prefix on the user message (KV-cache friendly)
    QString userMsg = "[" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "] "
                      + text + "\n" + contextBlock;

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
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
    });
    QFuture<QString> future = QtConcurrent::run([system, userMsg]() {
        return callDeepSeekStatic(system, userMsg);
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

    QNetworkAccessManager mgr;
    QNetworkRequest req;
    req.setUrl(QUrl(base + "/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    QNetworkReply *reply = mgr.post(req, QJsonDocument(body).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
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
