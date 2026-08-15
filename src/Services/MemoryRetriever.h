#pragma once
#include "FactModel.h"
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QJsonArray>

// =====================================================================
// MemoryRetriever — relevance-scored memory recall.
//
// Instead of always taking the newest N notes/events, score every memory
// entry against the current user message + conversation topic and return
// the most relevant ones. Old important memories become recallable; noise
// stays filtered out.
//
// score = relevance*0.45 + importance*0.25 + recency*0.20 + relationship*0.10
//
// MemoryKind (USER_FACT / SYSTEM_DATA / MEMORY_EVENT / MEMORY_SUMMARY)
// is defined in FactModel.h — this module reuses it.
// =====================================================================

struct MemoryEntry
{
    QString     content;
    QString     kindName;       // "USER_FACT" | "SYSTEM_DATA" | "MEMORY_EVENT" | "MEMORY_SUMMARY"
    double      confidence = 0.5;
    double      score = 0.0;    // computed by MemoryRetriever
    QDateTime   timestamp;
    QStringList tags;
    bool        rejected = false;
    bool        isEvent = false;    // true if it came from events[] (shared experience)

    static QString kindLabel(MemoryKind k)
    {
        switch (k) {
        case MemoryKind::UserFact:     return QStringLiteral("USER_FACT");
        case MemoryKind::SystemData:   return QStringLiteral("SYSTEM_DATA");
        case MemoryKind::MemoryEvent:  return QStringLiteral("MEMORY_EVENT");
        case MemoryKind::MemorySummary:return QStringLiteral("MEMORY_SUMMARY");
        }
        return QStringLiteral("MEMORY_SUMMARY");
    }
    static double kindConfidence(MemoryKind k)
    {
        switch (k) {
        case MemoryKind::UserFact:     return 1.0;
        case MemoryKind::SystemData:   return 0.95;
        case MemoryKind::MemoryEvent:  return 0.85;
        case MemoryKind::MemorySummary:return 0.5;
        }
        return 0.5;
    }
    static double kindImportance(MemoryKind k)
    {
        switch (k) {
        case MemoryKind::UserFact:     return 1.0;
        case MemoryKind::SystemData:   return 0.85;
        case MemoryKind::MemoryEvent:  return 0.75;
        case MemoryKind::MemorySummary:return 0.4;
        }
        return 0.4;
    }
};

class MemoryRetriever
{
public:
    // classify a raw note string into a MemoryKind
    static MemoryKind classify(const QString &note);

    // compute a relevance score between a memory and the current context
    static double relevanceScore(const QString &memoryContent, const QString &userMsg, const QString &topic);

    // full score: relevance*0.45 + importance*0.25 + recency*0.20 + relationship*0.10
    static double score(const QString &content, MemoryKind kind,
                        const QDateTime &ts, bool isEvent,
                        const QString &userMsg, const QString &topic);

    // tag overlap / keyword helper used by relevance
    static bool sharesTerms(const QString &a, const QString &b);
};
