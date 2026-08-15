#include "ProactiveScore.h"

int ProactiveScore::compute(const ProactiveInput &in)
{
    int score = 0;

    // ---- positive ----
    if (in.idleMs >= 30 * 60 * 1000)   score += 30; // long time no interaction
    if (in.justFinishedTask)            score += 20;
    if (in.lateNight && !in.gaming)     score += 20; // online late at night
    if (in.userMoodLow)                 score += 15;

    // ---- negative ----
    if (in.gaming)                      score -= 50;
    if (in.coding)                      score -= 40;
    if (in.justRefused)                 score -= 30;
    if (in.fullscreen)                  score -= 20;

    return score;
}
