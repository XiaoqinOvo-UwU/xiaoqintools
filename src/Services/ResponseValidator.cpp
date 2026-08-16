#include "ResponseValidator.h"
#include "FactFilter.h"
#include "ResponseRepair.h"
#include <QRegularExpression>

// final safety net: nothing that starts a <| control block may reach the UI.
// If one survives all earlier stages, drop the WHOLE block (up to the closing
// "|>", or the rest of the line when the closing marker is missing).
static QString dropResidualControlBlocks(const QString &s)
{
    QString out = s;
    int idx;
    while ((idx = out.indexOf(QStringLiteral("<|"))) >= 0) {
        const int end = out.indexOf(QStringLiteral("|>"), idx + 2);
        if (end >= 0) {
            out = out.left(idx) + out.mid(end + 2);
        } else {
            const int nl = out.indexOf('\n', idx);
            out = nl >= 0 ? (out.left(idx) + out.mid(nl)) : out.left(idx);
        }
    }
    return out.trimmed();
}

QString ResponseValidator::stripControlTokens(const QString &raw)
{
    // known control tokens with an OPTIONAL payload:
    //   <|ACT|> , <|ACT {json}|> , <|THINK|> , <|THINK {json}|> ,
    //   <|DELAY|> , <|DELAY 1.5|>
    static const QRegularExpression ctrlRe(
        "<\\|\\s*(ACT|THINK|DELAY)(?:\\s*\\{.*?\\}|\\s+[0-9.]+)?\\s*\\|>",
        QRegularExpression::DotMatchesEverythingOption);
    // legacy reasoning block <think>...</think>
    static const QRegularExpression thinkRe("<think>(.*?)</think>",
                                            QRegularExpression::DotMatchesEverythingOption);
    QString s = raw;
    s.remove(ctrlRe);
    s.remove(thinkRe);
    return dropResidualControlBlocks(s);
}

QString ResponseValidator::stripStageDirections(const QString &raw)
{
    QString s = raw;
    // full-width parens as literal chars (source is UTF-8; QRegularExpression
    // does NOT understand \uXXXX escapes — use \x{...} or raw chars)
    static const QRegularExpression parenRe(QStringLiteral("[（(][^（()]*[)）]"));
    static const QRegularExpression starRe(QStringLiteral("\\*[^*\\n]*\\*"));
    s.remove(parenRe);
    s.remove(starRe);
    // NOTE: no length-based line filtering here — natural replies are
    // unquoted and can be long; line-level narration filtering happens in
    // trimToDialog with a strict narration pattern.
    return s.trimmed();
}

// repair (not delete) fabricated observations:
//   - line has an observation phrase
//   - AND no verified fact text appears on the line
// -> rewrite via ResponseRepair; fall back to deletion when no rewrite exists
QString ResponseValidator::stripUnsupportedObservations(const QString &raw, const QString &factText)
{
    return stripUnsupportedObservationsImpl(raw, factText, nullptr, nullptr);
}

QString ResponseValidator::stripUnsupportedObservationsImpl(const QString &raw, const QString &factText,
                                                            int *repaired, int *dropped)
{
    const QStringList lines = raw.split('\n');
    QStringList kept;
    int nRepaired = 0, nDropped = 0;
    for (const QString &line : lines) {
        bool unsupported = false;
        for (const QString &ph : FactFilter::observationPhrases()) {
            if (line.contains(ph)) {
                bool supported = false;
                const QStringList facts = factText.split('\n');
                for (const QString &f : facts) {
                    QString cleanF = f;
                    cleanF.remove(QRegularExpression("^-\\s*\\[[^\\]]+\\]\\s*"));
                    if (!cleanF.isEmpty() && line.contains(cleanF.left(qMax(3, cleanF.size() / 2)))) {
                        supported = true;
                        break;
                    }
                }
                if (!supported) { unsupported = true; break; }
            }
        }
        if (!unsupported) {
            kept << line;
            continue;
        }
        // try to repair into a natural uncertain expression first
        QString r = ResponseRepair::repairObservationLine(line);
        if (!r.isEmpty()) {
            kept << r;
            ++nRepaired;
        } else {
            ++nDropped; // last-resort deletion
        }
    }
    if (repaired) *repaired = nRepaired;
    if (dropped) *dropped = nDropped;
    return kept.join('\n').trimmed();
}

// keep dialog lines; drop only obvious standalone narration lines.
// Natural replies are UNQUOTED ("没关系啦") and can be long, so we must not
// drop long lines just because they lack quotes — that would silently kill
// replies (and idle chat entirely). We only drop lines that read like
// narration: verb-led, quote-less, and ending with 。.
QString ResponseValidator::trimToDialog(const QString &raw)
{
    static const QRegularExpression narrRe(QStringLiteral(
        "^(我|他|她|它|你|大家|房间|空气|气氛|周围)[^，。！？]{2,20}"
        "(了|着|下|起|在|又|也|便|就|开始|继续|轻轻|默默|缓缓|低头|抬头|转身|露出|看着|听到|感到|觉得|想了|沉默|停顿|叹气|微笑|点头|摇头)"
        "[^\"“”]*。$"));
    QStringList kept;
    for (const QString &line : raw.split('\n')) {
        QString l = line.trimmed();
        if (l.isEmpty()) continue;
        bool hasQuote = l.contains(QString("\"")) || l.contains(QString("“")) || l.contains(QString("”"));
        if (hasQuote) { kept << l; continue; }
        // drop only high-confidence narration lines
        if (l.length() > 8 && l.length() <= 60 && narrRe.match(l).hasMatch())
            continue;
        kept << l;
    }
    return kept.join('\n').trimmed();
}

// drop lines where the AI describes a physical action it cannot do
// (抱抱/点外卖/走到你身边...). Keeps the AI in online/network mode.
QString ResponseValidator::stripPhysicalActions(const QString &raw)
{
    const QStringList lines = raw.split('\n');
    QStringList kept;
    for (const QString &line : lines) {
        bool bad = false;
        for (const QString &ph : FactFilter::physicalActionPhrases()) {
            if (line.contains(ph)) { bad = true; break; }
        }
        if (!bad) kept << line;
    }
    return kept.join('\n').trimmed();
}

ResponseValidator::Result ResponseValidator::validate(const QString &raw, const QString &verifiedFacts)
{
    Result r;
    QString s = raw;
    const QString before = s;

    s = stripControlTokens(s);
    s = stripStageDirections(s);

    // observation fabrication check (uses the fact block as support base).
    // Unsupported observations are REPAIRED, not just deleted.
    int repaired = 0, dropped = 0;
    const QString withoutObs = stripUnsupportedObservationsImpl(s, verifiedFacts, &repaired, &dropped);
    r.observationsRepaired = repaired;
    if (dropped > 0)
        r.observationsStripped = dropped;
    s = withoutObs;

    // online-mode: never let the AI describe physical body actions
    s = stripPhysicalActions(s);

    s = trimToDialog(s);
    s = s.trimmed();
    // final safety filter: no control token may reach the UI
    s = dropResidualControlBlocks(s);

    r.text = s;
    r.changed = (s != before);
    return r;
}
