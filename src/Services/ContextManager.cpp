#include "ContextManager.h"
#include "MemoryRetriever.h"
#include <QSet>
#include <QRegularExpression>
#include <cmath>

// ---- id ----
QString ContextManager::nextId(const QString &prefix)
{
    return QString("%1_%2").arg(prefix).arg(++m_idCounter);
}

bool ContextManager::hasFactWithContent(const QString &content) const
{
    const QString c = content.trimmed();
    for (const Fact &f : m_facts)
        if (f.content.trimmed() == c) return true;
    return false;
}

// ---- collection ----
void ContextManager::addFact(const Fact &f)
{
    if (f.content.trimmed().isEmpty()) return;
    if (hasFactWithContent(f.content)) return; // dedupe same wording
    m_facts.append(f);
    if (m_facts.size() > 300) { // session bound
        // drop oldest non-rejected fact (never drop corrections history)
        for (int i = 0; i < m_facts.size(); ++i) {
            if (!m_facts.at(i).rejected) {
                m_facts.removeAt(i);
                break;
            }
        }
    }
}

void ContextManager::addUserMessage(const QString &text, const QStringList &tags)
{
    QString t = text.trimmed();
    if (t.isEmpty()) return;
    // skip pure negations (they are corrections, not new facts)
    static const QRegularExpression negRe("^(没有|不是|不对|才没有|我才没|我没有|没在|没玩|不是的|你错了|说错了|瞎说)");
    if (negRe.match(t).hasMatch()) return;

    Fact f;
    f.id = nextId("usr");
    f.content = t;
    f.source = FactSource::UserMessage;
    f.confidence = 1.0;
    f.tags = tags;
    addFact(f);
}

void ContextManager::addSystemData(const QString &content, double confidence, const QStringList &tags)
{
    Fact f;
    f.id = nextId("sys");
    f.content = content;
    f.source = FactSource::SystemData;
    f.confidence = qBound(0.0, confidence, 1.0);
    f.tags = tags;
    addFact(f);
}

void ContextManager::addMemoryFact(const QString &content, MemoryKind kind, const QStringList &tags)
{
    Fact f;
    f.id = nextId("mem");
    f.content = content;
    f.source = FactSource::Memory;
    f.memKind = kind;
    f.confidence = MemoryEntry::kindConfidence(kind); // USER_FACT 1.0 ... SUMMARY 0.5
    f.tags = tags;
    addFact(f);
}

void ContextManager::addHypothesis(const QString &content, double confidence, const QStringList &tags)
{
    Fact f;
    f.id = nextId("hyp");
    f.content = content;
    f.source = FactSource::Inference;
    f.confidence = qBound(0.0, confidence, 1.0);
    f.tags = tags;
    addFact(f);
}

void ContextManager::clearSession()
{
    m_facts.clear();
    m_idCounter = 0;
    m_totalCorrections = 0;
}

// ---- queries ----
QList<Fact> ContextManager::factsBySource(FactSource s) const
{
    QList<Fact> out;
    for (const Fact &f : m_facts)
        if (f.source == s) out.append(f);
    return out;
}

QList<Fact> ContextManager::validFacts() const
{
    QList<Fact> out;
    for (const Fact &f : m_facts)
        if (f.isFact()) out.append(f);
    return out;
}

QList<Fact> ContextManager::hypotheses() const
{
    QList<Fact> out;
    for (const Fact &f : m_facts)
        if (f.isHypothesis()) out.append(f);
    return out;
}

QList<Fact> ContextManager::retrieveMemories(const QString &userMsg, const QString &topic, int max) const
{
    QList<Fact> out;
    for (const Fact &f : m_facts) {
        if (!f.isFact() || f.source != FactSource::Memory) continue;
        Fact scored = f;
        scored.retrievalScore = MemoryRetriever::score(
            f.content, f.memKind, f.timestamp, f.tags.contains("memory_event"),
            userMsg, topic);
        out.append(scored);
    }
    // sort by score desc, take top `max`
    std::sort(out.begin(), out.end(), [](const Fact &a, const Fact &b) {
        return a.retrievalScore > b.retrievalScore;
    });
    if (out.size() > max)
        out = out.mid(0, max);
    return out;
}

QString ContextManager::recallReport(const QString &userMsg, const QString &topic, int max) const
{
    QList<Fact> hits = retrieveMemories(userMsg, topic, max);
    QStringList lines;
    lines << QString("[recall] query='%1' topic='%2' -> %3 memories")
                 .arg(userMsg.left(30), topic.left(20)).arg(hits.size());
    for (const Fact &f : hits)
        lines << QString("  %1 | kind=%2 | score=%3 | conf=%4 | %5")
                     .arg(f.id, f.memKindName())
                     .arg(f.retrievalScore, 0, 'f', 2)
                     .arg(f.confidence)
                     .arg(f.content.left(50));
    return lines.join("\n");
}

QStringList ContextManager::factIds() const
{
    QStringList out;
    for (const Fact &f : m_facts)
        out << f.id;
    return out;
}

// ---- correction ----
QStringList ContextManager::detectCorrection(const QString &userText, const QString &lastAiReply)
{
    QStringList hit;
    const QString t = userText.trimmed();

    // 1) explicit negation phrases
    static const QStringList negs = {
        "没有", "不是", "不对", "才没有", "我才没", "我没有", "没在", "没玩",
        "不是的", "你错了", "说错了", "瞎说", "别乱猜", "乱说", "猜错了",
        "并没有", "并没有啊", "哪有", "哪来的",
    };
    bool negated = false;
    for (const QString &n : negs)
        if (t.contains(n)) { negated = true; break; }

    // 2) "你说我..." pattern: "你说我在玩游戏，其实没有"
    static const QRegularExpression sayRe("你说(?:我)?(?:在|又|正|刚)?([^，。！？,]{1,12})");
    QString claimedSubject;
    auto sm = sayRe.match(t);
    if (sm.hasMatch())
        claimedSubject = sm.captured(1).trimmed();

    if (!negated && claimedSubject.isEmpty())
        return hit; // no negation signal at all

    // scan the previous AI reply for claims we can map to
    QString ai = lastAiReply;
    for (const Fact &f : m_facts) {
        if (f.rejected) continue;
        if (f.source == FactSource::Inference) continue; // hypotheses aren't claimed facts
        const QString content = f.content;
        // tag overlap with the negated subject
        bool matches = false;
        for (const QString &tag : f.tags) {
            if (!tag.isEmpty() && t.contains(tag)) { matches = true; break; }
        }
        // "你说我<subject>" pattern: subject must be a meaningful chunk (>=2 chars)
        if (!matches && !claimedSubject.isEmpty() && claimedSubject.size() >= 2 &&
            (content.contains(claimedSubject) || claimedSubject.contains(content.left(qMin(4, content.size()))) ||
             (content.size() >= 4 && t.contains(content.left(qMin(4, content.size()))))))
            matches = true;
        // the AI literally echoed a known fact in its last reply, and the user
        // now negates it: only match when the AI text contains a substantial
        // slice (>=4 chars) of the fact content
        if (!matches && !ai.isEmpty() && content.size() >= 4) {
            const QString slice = content.left(qMin(6, content.size()));
            if (slice.size() >= 4 && ai.contains(slice))
                matches = true;
        }
        if (matches) hit << f.id;
    }
    return hit;
}

void ContextManager::rejectFact(const QString &id)
{
    for (Fact &f : m_facts) {
        if (f.id == id) {
            f.rejected = true;
            f.correctionCount++;
            m_totalCorrections++;
            break;
        }
    }
}

void ContextManager::rejectAllWithTag(const QString &tag)
{
    for (Fact &f : m_facts) {
        if (f.rejected) continue;
        if (f.tags.contains(tag)) {
            f.rejected = true;
            f.correctionCount++;
            m_totalCorrections++;
        }
    }
}

int ContextManager::correctionCount(const QString &id) const
{
    for (const Fact &f : m_facts)
        if (f.id == id) return f.correctionCount;
    return 0;
}

// ---- topic scoring ----
QList<ContextManager::TopicScore> ContextManager::rankTopics(
    const QStringList &unfinishedTopics,
    const QStringList &interests,
    const QStringList &recentChat,
    const QStringList &events,
    const QStringList &userRecentFacts) const
{
    QList<TopicScore> out;
    QHash<QString, TopicScore> map; // topic -> score

    const QDateTime now = QDateTime::currentDateTime();

    auto bump = [&](const QString &raw, double weight, const QString &source) {
        QString topic = raw.trimmed();
        // strip decorations like "- [08-20 22:10] " or leading "- "
        topic.remove(QRegularExpression("^-\\s*"));
        topic.remove(QRegularExpression("^\\[[0-9\\-: ]+\\]\\s*"));
        topic.remove(QRegularExpression("^-\\s*"));
        topic = topic.trimmed();
        if (topic.isEmpty() || topic.length() > 60) return;

        TopicScore &ts = map[topic];
        ts.topic = topic;
        ts.source = source;
        ts.score += weight;
        ts.lastTime = now;
    };

    // 1) unfinished topics — user cares, most recent first (weight 3, time decay)
    for (int i = 0; i < unfinishedTopics.size(); ++i)
        bump(unfinishedTopics.at(i), 3.0 / (i + 1), "unfinished");

    // 2) interests — stable preference (weight 1.5)
    for (int i = 0; i < interests.size(); ++i)
        bump(interests.at(i), 1.5 / (i + 1), "interest");

    // 3) recent chat — what was just discussed (weight 2, newest highest)
    for (int i = 0; i < recentChat.size(); ++i) {
        QString line = recentChat.at(recentChat.size() - 1 - i);
        line.remove(QRegularExpression("^用户:\\s*"));
        line.remove(QRegularExpression("^[^:]+:\\s*")); // strip "AI名: "
        bump(line, 2.0 / (i + 1), "recent");
    }

    // 4) events — shared memories (weight 2)
    for (int i = 0; i < events.size(); ++i)
        bump(events.at(i), 2.0 / (i + 1), "event");

    // 5) user's own recent stated facts (weight 2.5 — most reliable)
    for (int i = 0; i < userRecentFacts.size(); ++i)
        bump(userRecentFacts.at(i), 2.5 / (i + 1), "user_said");

    // convert map -> list and sort desc
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        out.append(it.value());
    std::sort(out.begin(), out.end(), [](const TopicScore &a, const TopicScore &b) {
        return a.score > b.score;
    });
    return out;
}

// ---- prompt sections ----
QString ContextManager::factsSection(int maxFacts, const QString &userMsg, const QString &topic) const
{
    // source priority: system_data (real-time truth) > user_message (stated) > memory
    // and never flood the block with raw user quotes — cap user_message.
    QList<Fact> sys, usr;
    for (const Fact &f : m_facts) {
        if (!f.isFact()) continue;
        if (f.source == FactSource::SystemData) sys.append(f);
        else if (f.source == FactSource::UserMessage) usr.append(f);
    }
    auto byConf = [](const Fact &a, const Fact &b) { return a.confidence > b.confidence; };
    std::sort(sys.begin(), sys.end(), byConf);
    std::sort(usr.begin(), usr.end(), byConf);
    // memory: reuse the score-based retriever (single source of truth for ranking)
    QList<Fact> mem = retrieveMemories(userMsg, topic, 20);

    QStringList lines;
    int budget = maxFacts;
    auto take = [&](QList<Fact> &list, int max) {
        int n = qMin((int)list.size(), max);
        for (int i = 0; i < n && budget > 0; ++i, --budget) {
            const Fact &f = list.at(i);
            // show memory source type so the AI knows how reliable it is
            if (f.source == FactSource::Memory)
                lines << QString("- [%1] %2").arg(f.memKindName(), f.content);
            else
                lines << QString("- [%1] %2").arg(f.sourceName(), f.content);
        }
    };
    // memory facts are the companion's long-term knowledge — give them the
    // largest share. system data up to 3, user up to 2, rest goes to memory.
    take(sys, 3);
    take(usr, 2);
    take(mem, maxFacts);
    // if budget still remains and more system/user facts exist, top up
    if (budget > 0) {
        take(sys, maxFacts);
        take(usr, maxFacts);
    }
    return lines.join("\n");
}

QString ContextManager::hypothesesSection(int maxHypotheses) const
{
    QList<Fact> hy = hypotheses();
    std::sort(hy.begin(), hy.end(), [](const Fact &a, const Fact &b) {
        return a.confidence > b.confidence;
    });
    QStringList lines;
    int n = qMin(hy.size(), maxHypotheses);
    for (int i = 0; i < n; ++i)
        lines << QString("- [推测] %1（仅猜测，必须用询问语气）").arg(hy.at(i).content);
    return lines.join("\n");
}
