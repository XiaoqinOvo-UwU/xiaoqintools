#pragma once
#include <QString>
#include <QDateTime>

// =====================================================================
// ConversationState — short-term session memory that keeps the AI on the
// current topic and emotional track across turns. Lives ONLY for the
// current chat session; never written to long-term memory.
//
// State fields:
//   topic             — what we are talking about right now
//   userEmotion       — detected emotion of the user this turn
//   assistantIntent   — what the AI decided to do (comfort/ask/listen/...)
//   relationshipMode  — comfort/support vs casual vs guidance
//   unresolvedIssue   — an emotional event still needing attention
// =====================================================================
struct ConversationState
{
    QString topic;               // "用户感到被忽视"
    QString userEmotion;         // happy/sad/lonely/stressed/tired/normal/...
    QString assistantIntent;     // "安慰并确认关系"
    QString relationshipMode;    // "comfort" | "casual" | "guidance" | "neutral"
    QString unresolvedIssue;     // non-empty while an emotional issue is open
    QDateTime lastUpdate;

    void clear() { *this = ConversationState(); }

    bool hasUnresolvedEmotion() const
    {
        return !unresolvedIssue.isEmpty();
    }

    // prompt-ready summary (Chinese, compact)
    QString summary() const
    {
        QStringList parts;
        if (!topic.isEmpty())
            parts << "当前话题：" + topic;
        if (!userEmotion.isEmpty())
            parts << "用户情绪：" + userEmotion;
        if (!assistantIntent.isEmpty())
            parts << "你正在做的事：" + assistantIntent;
        if (!relationshipMode.isEmpty())
            parts << "当前关系模式：" + relationshipMode;
        if (hasUnresolvedEmotion())
            parts << "未解决的情绪事件：" + unresolvedIssue;
        return parts.join("\n");
    }
};
