#include "AppCore.h"

#include <QCoreApplication>
#include <QDir>

AppCore &AppCore::instance()
{
    static AppCore inst;
    return inst;
}

AppCore::AppCore(QObject *parent)
    : QObject(parent)
{
}

void AppCore::navigate(const QString &page)
{
    Q_UNUSED(page)
    // Page switching is handled in QML via StackView; this hook exists for
    // future programmatic navigation / deep links.
}

void AppCore::setStatus(const QString &s)
{
    if (m_status != s) {
        m_status = s;
        emit statusTextChanged();
    }
}

void AppCore::showToast(const QString &msg)
{
    m_toastMsg = msg;
    m_toastSeq++;
    emit toastRequested();
}

bool AppCore::isProperLocation()
{
    QString dir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    // recognized locations:
    //  - installed: ...\Program Files\XiaoQinTools  (or Program Files (x86))
    //  - dev copy:  C:\XiaoQinTools\dist
    if (dir.contains("Program Files", Qt::CaseInsensitive) && dir.endsWith("XiaoQinTools"))
        return true;
    if (dir.compare("C:\\XiaoQinTools\\dist", Qt::CaseInsensitive) == 0)
        return true;
    return false;
}
