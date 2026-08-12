#pragma once
#include <QObject>
#include <QString>
#include <QStringList>

// Multi-AI contact manager: each contact has its own name, personality,
// avatar image and memory directory under %APPDATA%/XiaoQinTools/contacts/<id>/.
class ContactService : public QObject
{
    Q_OBJECT
public:
    explicit ContactService(QObject *parent = nullptr);

    // singleton accessor for C++ consumers (QML uses the context property)
    static ContactService &instance();

    // list of "id|name|hasAvatar" lines
    Q_INVOKABLE QStringList contactList();

    // add a contact, switch to it, return its id
    Q_INVOKABLE QString addContact(const QString &name, const QString &personality);
    Q_INVOKABLE void removeContact(const QString &id);

    // current contact switching
    Q_INVOKABLE QString currentId();
    Q_INVOKABLE void setCurrent(const QString &id);
    Q_INVOKABLE QString currentName();
    Q_INVOKABLE QString currentPersonality();
    Q_INVOKABLE void setCurrentName(const QString &v);
    Q_INVOKABLE void setCurrentPersonality(const QString &v);

    // avatar: copy a local image into the contact dir, return stored path
    Q_INVOKABLE QString setCurrentAvatar(const QString &srcPath);
    Q_INVOKABLE QString currentAvatarPath();

    // helpers for AiService (per-contact dirs)
    QString contactDir(const QString &id) const;
    QString contactMemoryPath(const QString &id) const;
    Q_INVOKABLE QString contactAvatarPath(const QString &id) const;

    // ensure a default contact exists (first run)
    void ensureDefault();

signals:
    void contactsChanged();

private:
    void load();
    void save();
    QStringList ids() const;

    struct Contact {
        QString id;
        QString name;
        QString personality;
    };
    QList<Contact> m_contacts;
    QString m_currentId;
};
