#include "MoodService.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

MoodService::MoodService(QObject *parent)
    : QObject(parent)
{
}

QString MoodService::diaryPath() const
{
    // Keep the 杂货铺 convention.
    const QString dir = "C:/deepseek杂货铺/心情日记";
    QDir().mkpath(dir);
    return dir + "/心情日记.txt";
}

QStringList MoodService::moodOptions() const
{
    return {
        "好耶！开心捏~",
        "平平淡淡~",
        "呜呜有点难过 QwQ",
        "气鼓鼓的哼！",
        "累瘫了不想动",
        "烦烦的想打人 (>_<)",
        "今天想摆烂~",
        "元气满满冲鸭！",
    };
}

bool MoodService::recordMood(const QString &mood)
{
    QString line = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + "  " + mood + "\r\n";
    QFile f(diaryPath());
    if (!f.open(QIODevice::Append | QIODevice::Text)) return false;
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << line;
    f.close();
    return true;
}

QString MoodService::history()
{
    QFile f(diaryPath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return "还没有心情记录哦~";
    QString all = QString::fromUtf8(f.readAll());
    f.close();
    return all.trimmed().isEmpty() ? "还没有心情记录哦~" : all;
}
