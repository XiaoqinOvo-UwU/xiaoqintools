#pragma once
#include <QObject>
#include <QString>

// AppCore is the single bridge between QML and C++ business logic.
// It owns the services and exposes a clean API surface to QML.
class AppCore : public QObject
{
    Q_OBJECT
public:
    static AppCore &instance();

    // navigation
    Q_INVOKABLE void navigate(const QString &page);

    // status line for the right side / status area
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    QString statusText() const { return m_status; }
    Q_INVOKABLE void setStatus(const QString &s);

    // island toast
    Q_INVOKABLE void showToast(const QString &msg);
    Q_PROPERTY(QString toastMessage READ toastMessage NOTIFY toastRequested)
    Q_PROPERTY(int toastSeq READ toastSeq NOTIFY toastRequested)
    QString toastMessage() const { return m_toastMsg; }
    int toastSeq() const { return m_toastSeq; }

    // returns true if the exe runs from a recognized install location
    Q_INVOKABLE bool isProperLocation();

signals:
    void statusTextChanged();
    void toastRequested();

private:
    explicit AppCore(QObject *parent = nullptr);
    QString m_status = "就绪";
    QString m_toastMsg;
    int m_toastSeq = 0;
};
