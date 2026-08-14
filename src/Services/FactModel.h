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

struct Fact
{
    QString     id;               // stable id, e.g. "sys_foreground_1650000000"
    QString     content;          // "用户正在玩Minecraft"
    FactSource  source = FactSource::Inference;
    double      confidence = 0.5; // 0.0 ~ 1.0
    QDateTime   timestamp = QDateTime::currentDateTime();
    bool        rejected = false;        // user corrected it -> excluded everywhere
    int         correctionCount = 0;     // how many times it was corrected
    QStringList tags;                    // e.g. {"game", "minecraft", "activity"}

    bool isFact() const { return !rejected && source != FactSource::Inference; }
    bool isHypothesis() const { return !rejected && source == FactSource::Inference; }

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
