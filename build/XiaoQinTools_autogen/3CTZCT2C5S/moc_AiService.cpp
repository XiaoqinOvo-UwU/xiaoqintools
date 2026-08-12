/****************************************************************************
** Meta object code from reading C++ file 'AiService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Services/AiService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AiService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9AiServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto AiService::qt_create_metaobjectdata<qt_meta_tag_ZN9AiServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AiService",
        "chatReply",
        "",
        "text",
        "thinkingReady",
        "greetingReady",
        "greeting",
        "shouldGreetToday",
        "markGreeted",
        "sendMessage",
        "memoryReport",
        "uptimeText",
        "generateGreeting",
        "isGameRunning",
        "idleChat",
        "userName",
        "avatarChar",
        "aiName",
        "aiPersonality",
        "setUserName",
        "v",
        "setAvatarChar",
        "setAiName",
        "setAiPersonality",
        "apiBaseUrl",
        "apiModel",
        "apiKey",
        "setApiBaseUrl",
        "setApiModel",
        "setApiKey",
        "setUserAvatar",
        "srcPath",
        "setAiAvatar",
        "userAvatarPath",
        "aiAvatarPath",
        "recordSessionStart",
        "recordSessionEnd"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'chatReply'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'thinkingReady'
        QtMocHelpers::SignalData<void(QString)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Signal 'greetingReady'
        QtMocHelpers::SignalData<void(QString)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'greeting'
        QtMocHelpers::MethodData<QString()>(6, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'shouldGreetToday'
        QtMocHelpers::MethodData<bool()>(7, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'markGreeted'
        QtMocHelpers::MethodData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'sendMessage'
        QtMocHelpers::MethodData<void(const QString &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'memoryReport'
        QtMocHelpers::MethodData<QString()>(10, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'uptimeText'
        QtMocHelpers::MethodData<QString()>(11, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'generateGreeting'
        QtMocHelpers::MethodData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'isGameRunning'
        QtMocHelpers::MethodData<bool()>(13, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'idleChat'
        QtMocHelpers::MethodData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'userName'
        QtMocHelpers::MethodData<QString()>(15, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'avatarChar'
        QtMocHelpers::MethodData<QString()>(16, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'aiName'
        QtMocHelpers::MethodData<QString()>(17, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'aiPersonality'
        QtMocHelpers::MethodData<QString()>(18, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'setUserName'
        QtMocHelpers::MethodData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setAvatarChar'
        QtMocHelpers::MethodData<void(const QString &)>(21, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setAiName'
        QtMocHelpers::MethodData<void(const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setAiPersonality'
        QtMocHelpers::MethodData<void(const QString &)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'apiBaseUrl'
        QtMocHelpers::MethodData<QString()>(24, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'apiModel'
        QtMocHelpers::MethodData<QString()>(25, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'apiKey'
        QtMocHelpers::MethodData<QString()>(26, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'setApiBaseUrl'
        QtMocHelpers::MethodData<void(const QString &)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setApiModel'
        QtMocHelpers::MethodData<void(const QString &)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setApiKey'
        QtMocHelpers::MethodData<void(const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'setUserAvatar'
        QtMocHelpers::MethodData<QString(const QString &)>(30, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 31 },
        }}),
        // Method 'setAiAvatar'
        QtMocHelpers::MethodData<QString(const QString &)>(32, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 31 },
        }}),
        // Method 'userAvatarPath'
        QtMocHelpers::MethodData<QString()>(33, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'aiAvatarPath'
        QtMocHelpers::MethodData<QString()>(34, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'recordSessionStart'
        QtMocHelpers::MethodData<void()>(35, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'recordSessionEnd'
        QtMocHelpers::MethodData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AiService, qt_meta_tag_ZN9AiServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AiService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AiServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AiServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9AiServiceE_t>.metaTypes,
    nullptr
} };

void AiService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AiService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->chatReply((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->thinkingReady((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->greetingReady((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: { QString _r = _t->greeting();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 4: { bool _r = _t->shouldGreetToday();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 5: _t->markGreeted(); break;
        case 6: _t->sendMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: { QString _r = _t->memoryReport();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 8: { QString _r = _t->uptimeText();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 9: _t->generateGreeting(); break;
        case 10: { bool _r = _t->isGameRunning();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->idleChat(); break;
        case 12: { QString _r = _t->userName();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 13: { QString _r = _t->avatarChar();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->aiName();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { QString _r = _t->aiPersonality();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 16: _t->setUserName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->setAvatarChar((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->setAiName((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->setAiPersonality((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: { QString _r = _t->apiBaseUrl();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 21: { QString _r = _t->apiModel();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 22: { QString _r = _t->apiKey();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 23: _t->setApiBaseUrl((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->setApiModel((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 25: _t->setApiKey((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 26: { QString _r = _t->setUserAvatar((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 27: { QString _r = _t->setAiAvatar((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 28: { QString _r = _t->userAvatarPath();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 29: { QString _r = _t->aiAvatarPath();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 30: _t->recordSessionStart(); break;
        case 31: _t->recordSessionEnd(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AiService::*)(QString )>(_a, &AiService::chatReply, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiService::*)(QString )>(_a, &AiService::thinkingReady, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AiService::*)(QString )>(_a, &AiService::greetingReady, 2))
            return;
    }
}

const QMetaObject *AiService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AiService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9AiServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AiService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 32;
    }
    return _id;
}

// SIGNAL 0
void AiService::chatReply(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void AiService::thinkingReady(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void AiService::greetingReady(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP
