#pragma once
#include <QString>

// =====================================================================
// ResponseValidator — mandatory post-processing gate for ALL LLM output
// (sendMessage / idleChat / greeting / summarizer). Nothing reaches the
// UI without passing through here.
//
// Checks:
//  1. strip <think> blocks, <|ACT|>, <|DELAY|> control tokens
//  2. strip stage directions: （动作）、*动作*、standalone narration lines
//  3. strip unsupported fabricated observations ("我看到你..." without data)
//  4. enforce dialog-only output (no role scripts, no narration)
// =====================================================================
class ResponseValidator
{
public:
    struct Result {
        QString text;       // cleaned output
        bool    changed;    // true if anything was stripped
        int     stagedStripped = 0;
        int     observationsStripped = 0;
    };

    // main entry: `verifiedFacts` = the [确定] fact block text used in the
    // prompt (used to decide whether an observation is supported).
    static Result validate(const QString &raw, const QString &verifiedFacts);

    // lightweight helpers (public for reuse/tests)
    static QString stripControlTokens(const QString &raw);   // think/ACT/DELAY
    static QString stripStageDirections(const QString &raw); // (xx) *xx* narration
    static QString stripUnsupportedObservations(const QString &raw, const QString &factText);
    static QString trimToDialog(const QString &raw);         // drop quote-less narration lines
};
