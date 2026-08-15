#pragma once
#include <QString>
#include <QDateTime>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

// =====================================================================
// FactModel — the single source-of-truth definition for every piece of
// information the companion AI knows about the user / system.
//
// Rule: only UserMessage / SystemData / Memory facts may be treated as
// facts. Inference (Hypothesis) may be referenced but ONLY in question
// form. Rejected facts are excluded from prompts entirely.
// =====================================================================

enum class FactSource {
    UserMessage,   // user explicitly said it
    SystemData,    // real detection from the computer (foreground, processes, time...)
    Memory,        // long-term memory notes / events
    Inference      // AI guess — hypothesis only, never a fact
};

// memory sub-type — which long-term bucket this memory came from
enum class MemoryKind {
    UserFact,      // user told the AI directly   -> confidence 1.0
    SystemData,    // real detection               -> confidence 0.95
    MemoryEvent,   // shared experience            -> confidence 0.85
    MemorySummary  // LLM auto-summary             -> confidence 0.5
};

struct Fact
{
    QString     id;               // stable id, e.g. "sys_foreground_1650000000"
    QString     content;          // "用户正在玩Minecraft"
    FactSource  source = FactSource::Inference;
    MemoryKind  memKind = MemoryKind::MemorySummary; // valid when source==Memory
    double      confidence = 0.5; // 0.0 ~ 1.0
    QDateTime   timestamp = QDateTime::currentDateTime();
    bool        rejected = false;        // user corrected it -> excluded everywhere
    int         correctionCount = 0;     // how many times it was corrected
    QStringList tags;                    // e.g. {"game", "minecraft", "activity"}
    double      retrievalScore = 0.0;    // MemoryRetriever score (prompt ordering)

    bool isFact() const { return !rejected && source != FactSource::Inference; }
    bool isHypothesis() const { return !rejected && source == FactSource::Inference; }

    QString memKindName() const
    {
        switch (memKind) {
        case MemoryKind::UserFact:     return "USER_FACT";
        case MemoryKind::SystemData:   return "SYSTEM_DATA";
        case MemoryKind::MemoryEvent:  return "MEMORY_EVENT";
        case MemoryKind::MemorySummary:return "MEMORY_SUMMARY";
        }
        return "MEMORY_SUMMARY";
    }

    QString sourceName() const
    {
        switch (source) {
        case FactSource::UserMessage: return "user_message";
        case FactSource::SystemData:  return "system_data";
        case FactSource::Memory:      return "memory";
        case FactSource::Inference:   return "inference";
        }
        return "unknown";
    }

    QJsonObject toJson() const
    {
        QJsonObject o;
        o.insert("id", id);
        o.insert("content", content);
        o.insert("source", sourceName());
        o.insert("confidence", confidence);
        o.insert("timestamp", timestamp.toString(Qt::ISODate));
        o.insert("rejected", rejected);
        o.insert("corrections", correctionCount);
        o.insert("tags", QJsonArray::fromStringList(tags));
        return o;
    }
};
