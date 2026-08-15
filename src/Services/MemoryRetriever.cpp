#include "MemoryRetriever.h"
#include <QSet>
#include <QStringList>
#include <QRegularExpression>
#include <QTime>
#include <cmath>

// classify a raw memory note into a kind.
// Heuristics: auto-summaries usually contain time patterns like （yyyy-MM-dd） or
// "休息了约 N 小时"; shared-experience events are tagged separately by caller.
MemoryKind MemoryRetriever::classify(const QString &note)
{
    const QString n = note.trimmed();
    // LLM auto-summaries: machine-generated meta-notes (NOT plain timestamps —
    // user notes also carry （date time） suffixes and must stay classified by content)
    static const QRegularExpression autoRe("休息了约|本次开机|上次会话结束|自动生成|系统记录|每日摘要");
    if (autoRe.match(n).hasMatch())
        return MemoryKind::MemorySummary;
    // explicit user statements: first-person-ish or direct user descriptions
    static const QStringList userMarks = { "用户喜欢", "用户使用", "用户有", "用户是",
                                           "我喜欢", "我是", "用户常", "用户习惯", "用户作息" };
    for (const QString &m : userMarks)
        if (n.contains(m)) return MemoryKind::UserFact;
    // fall back: plain notes with no strong signal stay a summary
    return MemoryKind::MemorySummary;
}

bool MemoryRetriever::sharesTerms(const QString &a, const QString &b)
{
    if (a.isEmpty() || b.isEmpty()) return false;
    // direct substring is the strongest signal
    if (a.contains(b) || b.contains(a)) return true;
    // bound input to avoid O(n^2) blowups on long messages
    const QString sa = a.simplified().left(120);
    const QString sb = b.simplified().left(120);
    if (sa.size() < 2 || sb.size() < 2) return false;
    // shared CJK bigrams via hash set (approx tokenization for Chinese)
    QSet<QString> bigA;
    for (int i = 0; i + 1 < sa.size(); ++i) {
        const QChar c0 = sa.at(i), c1 = sa.at(i + 1);
        if (c0.unicode() > 0x2E80 && c1.unicode() > 0x2E80)
            bigA.insert(QString(c0) + c1);
    }
    if (bigA.isEmpty()) return false;
    int hits = 0;
    for (int i = 0; i + 1 < sb.size(); ++i) {
        const QChar c0 = sb.at(i), c1 = sb.at(i + 1);
        if (c0.unicode() > 0x2E80 && c1.unicode() > 0x2E80) {
            if (bigA.contains(QString(c0) + c1)) {
                if (++hits >= 2) return true;
            }
        }
    }
    return false;
}

double MemoryRetriever::relevanceScore(const QString &memoryContent, const QString &userMsg, const QString &topic)
{
    double s = 0.0;
    if (sharesTerms(memoryContent, userMsg)) s += 0.6;
    if (!topic.isEmpty() && sharesTerms(memoryContent, topic)) s += 0.4;
    // time-of-day heuristic: memories mentioning night/hour match late chats
    if (memoryContent.contains("深夜") || memoryContent.contains("凌晨")) {
        int h = QTime::currentTime().hour();
        if (h >= 22 || h < 6) s += 0.25;
    }
    return qBound(0.0, s, 1.0);
}

double MemoryRetriever::score(const QString &content, MemoryKind kind,
                              const QDateTime &ts, bool isEvent,
                              const QString &userMsg, const QString &topic)
{
    const double relevance = relevanceScore(content, userMsg, topic);
    const double importance = MemoryEntry::kindImportance(kind);
    // recency: decay over ~14 days
    const qint64 ageDays = ts.isValid() ? qMax<qint64>(0, ts.daysTo(QDateTime::currentDateTime())) : 7;
    const double recency = qBound(0.0, 1.0 - ageDays / 14.0, 1.0);
    // relationship: shared experiences with the AI weigh more
    const double relationship = isEvent ? 0.8 : 0.4;

    return relevance * 0.45 + importance * 0.25 + recency * 0.20 + relationship * 0.10;
}
