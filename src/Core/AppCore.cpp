#include "AppCore.h"

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
