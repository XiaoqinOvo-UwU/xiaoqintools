#include "MemoryConflictManager.h"
#include "MemoryImportanceEvaluator.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>

QString MemoryConflictManager::m_path;
QJsonObject MemoryConflictManager::m_ledger;
QSet<QString> MemoryConflictManager::m_deprecatedCache;
bool MemoryConflictManager::m_dirty = false;

void MemoryConflictManager::setLedgerPath(const QString &path)
{
    m_path = path;
    reload();
}

QString MemoryConflictManager::ledgerPath() { return m_path; }

// strip trailing "（yyyy-MM-dd HH:mm）" suffix and any surrounding spaces
QString MemoryConflictManager::normalize(const QString &content)
{
    static const QRegularExpression suffixRe("[（(][0-9]{4}-[0-9]{2}-[0-9]{2}[ ][0-9:]+[)）]\\s*$");
    QString n = content.simplified();
    n.remove(suffixRe);
    n = n.trimmed();
    return n;
}

bool MemoryConflictManager::containsUpdateSignal(const QString &userText)
{
    static const QStringList updateSignals = {
        "不玩", "不喝", "不打了", "不看了", "不读了", "不追", "不做了",
        "不写了", "不用了", "不再", "退坑", "卸载", "删了", "戒了",
        "放弃了", "改玩", "换游戏", "开始玩", "现在只玩", "早就不",
        "已经不喜欢", "已经没玩", "没再玩", "最近不玩",
    };
    for (const QString &s : updateSignals)
        if (userText.contains(s)) return true;
    return false;
}

QString MemoryConflictManager::reasonForUpdate(const QString &userText)
{
    static const QStringList strong = { "退坑", "卸载", "删了", "戒了", "放弃了" };
    for (const QString &s : strong)
        if (userText.contains(s)) return "用户主动退出/放弃";
    static const QStringList replace = { "改玩", "换游戏", "现在只玩", "开始玩" };
    for (const QString &s : replace)
        if (userText.contains(s)) return "用户改变了偏好";
    return "用户主动更新";
}

// extract the topic subject from an update statement, e.g.
//   "我现在不玩Apex了" -> "Apex"
//   "我已经不喜欢喝可乐了" -> "可乐"
//   "我戒了游戏" -> "游戏"
static QString extractSubject(const QString &userText)
{
    QString t = userText.trimmed();
    // cut at the first update signal
    static const QStringList verbs = { "不玩", "不喝", "不打了", "不看了", "不读了",
                                       "不追", "不做了", "不写了", "不用了", "不再",
                                       "已经不喜欢", "不喜欢", "早就不", "没再玩" };
    int cut = -1, verbLen = 0;
    for (const QString &v : verbs) {
        int i = t.indexOf(v);
        if (i >= 0 && (cut < 0 || i < cut)) { cut = i; verbLen = v.size(); }
    }
    // also try single-verb "退坑/卸载/删了/戒了/放弃" (subject usually follows)
    static const QStringList drop = { "退坑", "卸载", "删了", "戒了", "放弃了", "放弃" };
    for (const QString &v : drop) {
        int i = t.indexOf(v);
        if (i >= 0 && (cut < 0 || i < cut)) { cut = i; verbLen = v.size(); }
    }
    if (cut < 0) return QString();

    QString sub = t.mid(cut + verbLen);
    // strip trailing particles / punctuation
    static const QRegularExpression tailRe("[了啊吧啦呢呀。！？,.!?]+\\s*$");
    sub.remove(tailRe);
    // strip leading temporal/emphatic words
    static const QRegularExpression headRe("^(我|现在|已经|以后|反正|真的|真的不|早就|最近|就|改|换)\\s*");
    sub.remove(headRe);
    sub = sub.trimmed();
    if (sub.isEmpty()) return QString();
    // a meaningful subject is at least 2 chars (avoid "它/这/那")
    return sub.size() >= 2 ? sub : QString();
}

MemoryConflictManager::Resolution MemoryConflictManager::resolve(
    const QString &userText, const QStringList &existingNotes)
{
    Resolution r;
    if (!containsUpdateSignal(userText)) return r;

    const QString subject = extractSubject(userText);
    if (subject.isEmpty()) return r;

    // 1) find old notes about this subject and deprecate them (never delete)
    for (const QString &note : existingNotes) {
        const QString n = normalize(note);
        if (n.isEmpty()) continue;
        if (n.contains(subject) && (n.contains("喜欢") || n.contains("爱") ||
                                    n.contains("玩") || n.contains("习惯") ||
                                    n.contains("常") || n.contains("追") ||
                                    n.contains("喝") || n.contains("用"))) {
            if (isDeprecated(note)) continue;      // already handled
            recordDeprecation(note, reasonForUpdate(userText), QString());
            r.deprecated << note;
            r.hadConflict = true;
        }
    }

    // 2) build the replacement active fact, e.g. "用户不玩Apex了"
    if (r.hadConflict || containsUpdateSignal(userText)) {
        QString cleaned = userText.simplified();
        static const QRegularExpression leadRe("^(我|我现在|我最近|其实|我觉得|我感觉|嗯|呃|那个)+\\s*");
        cleaned.remove(leadRe);
        cleaned = cleaned.trimmed();
        if (!cleaned.startsWith("用户"))
            cleaned = "用户" + cleaned;
        cleaned = normalize(cleaned);
        if (!cleaned.isEmpty() && cleaned.size() <= 40) {
            bool dup = false;
            for (const QString &note : existingNotes)
                if (normalize(note) == cleaned) { dup = true; break; }
            if (!dup && !r.created.contains(cleaned)) {
                r.created << cleaned;
                // record the replacement relation
                for (const QString &oldNote : r.deprecated)
                    recordDeprecation(oldNote, reasonForUpdate(userText), cleaned);
            }
        }
    }
    return r;
}

void MemoryConflictManager::recordDeprecation(const QString &oldNote,
                                              const QString &reason,
                                              const QString &replacedBy)
{
    QJsonArray conflicts = m_ledger.value("conflicts").toArray();
    const QString key = normalize(oldNote);
    // update in place if the same old note already has an entry
    for (int i = 0; i < conflicts.size(); ++i) {
        QJsonObject e = conflicts.at(i).toObject();
        if (e.value("old").toString() == key) {
            e.insert("status", "deprecated");
            e.insert("reason", reason);
            if (!replacedBy.isEmpty()) e.insert("replacedBy", replacedBy);
            e.insert("time", QDateTime::currentDateTime().toString(Qt::ISODate));
            conflicts[i] = e;
            m_ledger.insert("conflicts", conflicts);
            m_dirty = true;
            return;
        }
    }
    QJsonObject e;
    e.insert("old", key);
    e.insert("status", "deprecated");
    e.insert("reason", reason);
    if (!replacedBy.isEmpty()) e.insert("replacedBy", replacedBy);
    e.insert("time", QDateTime::currentDateTime().toString(Qt::ISODate));
    conflicts.append(e);
    if (conflicts.size() > 200) { // bound
        QJsonArray kept;
        for (int i = conflicts.size() - 200; i < conflicts.size(); ++i)
            kept.append(conflicts.at(i));
        conflicts = kept;
    }
    m_ledger.insert("conflicts", conflicts);
    m_deprecatedCache.insert(key);
    m_dirty = true;
}

bool MemoryConflictManager::isDeprecated(const QString &content)
{
    const QString key = normalize(content);
    if (m_deprecatedCache.contains(key)) return true;
    // fall back to scanning the ledger if the cache wasn't built yet
    const QJsonArray conflicts = m_ledger.value("conflicts").toArray();
    for (const QJsonValue &v : conflicts) {
        QJsonObject e = v.toObject();
        if (e.value("old").toString() == key &&
            e.value("status").toString() != "active") {
            m_deprecatedCache.insert(key);
            return true;
        }
    }
    return false;
}

QSet<QString> MemoryConflictManager::deprecatedSet()
{
    // warm the cache
    const QJsonArray conflicts = m_ledger.value("conflicts").toArray();
    for (const QJsonValue &v : conflicts) {
        QJsonObject e = v.toObject();
        if (e.value("status").toString() != "active")
            m_deprecatedCache.insert(e.value("old").toString());
    }
    return m_deprecatedCache;
}

QString MemoryConflictManager::replacementFor(const QString &content)
{
    const QString key = normalize(content);
    const QJsonArray conflicts = m_ledger.value("conflicts").toArray();
    for (const QJsonValue &v : conflicts) {
        QJsonObject e = v.toObject();
        if (e.value("old").toString() == key)
            return e.value("replacedBy").toString();
    }
    return QString();
}

void MemoryConflictManager::bumpUsage(const QString &content)
{
    const QString key = normalize(content);
    if (key.isEmpty()) return;
    QJsonObject usage = m_ledger.value("usage").toObject();
    usage.insert(key, usage.value(key).toInt() + 1);
    m_ledger.insert("usage", usage);
    QJsonObject last = m_ledger.value("lastUsed").toObject();
    last.insert(key, QDateTime::currentDateTime().toString(Qt::ISODate));
    m_ledger.insert("lastUsed", last);
    m_dirty = true;
}

int MemoryConflictManager::usageCount(const QString &content)
{
    const QString key = normalize(content);
    return m_ledger.value("usage").toObject().value(key).toInt();
}

double MemoryConflictManager::usageFrequency(const QString &content)
{
    const int c = usageCount(content);
    // soft curve: usage / (usage + 5) -> 0..1, ~0.83 at 25 recalls
    return double(c) / (double(c) + 5.0);
}

void MemoryConflictManager::reload()
{
    m_deprecatedCache.clear();
    m_ledger = QJsonObject();
    if (m_path.isEmpty()) return;
    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument d = QJsonDocument::fromJson(f.readAll());
        if (d.isObject()) m_ledger = d.object();
        f.close();
    }
    // warm cache from existing conflicts
    const QJsonArray conflicts = m_ledger.value("conflicts").toArray();
    for (const QJsonValue &v : conflicts) {
        QJsonObject e = v.toObject();
        if (e.value("status").toString() != "active")
            m_deprecatedCache.insert(e.value("old").toString());
    }
    m_dirty = false;
}

void MemoryConflictManager::save()
{
    if (!m_dirty || m_path.isEmpty()) return;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        f.write(QJsonDocument(m_ledger).toJson());
        f.close();
        m_dirty = false;
    }
}
