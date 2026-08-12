/****************************************************************************
** Meta object code from reading C++ file 'ProxyService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Services/ProxyService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ProxyService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12ProxyServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto ProxyService::qt_create_metaobjectdata<qt_meta_tag_ZN12ProxyServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ProxyService",
        "statusReady",
        "",
        "status",
        "isClashRunning",
        "isV2rayRunning",
        "isClashPortOpen",
        "isV2rayPortOpen",
        "launchClash",
        "launchV2ray",
        "launchAny",
        "hasClash",
        "hasV2ray",
        "copyToClipboard",
        "text",
        "quickStatus",
        "statusAsync",
        "clashExe",
        "v2rayExe"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'statusReady'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'isClashRunning'
        QtMocHelpers::MethodData<bool()>(4, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'isV2rayRunning'
        QtMocHelpers::MethodData<bool()>(5, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'isClashPortOpen'
        QtMocHelpers::MethodData<bool()>(6, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'isV2rayPortOpen'
        QtMocHelpers::MethodData<bool()>(7, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'launchClash'
        QtMocHelpers::MethodData<bool()>(8, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'launchV2ray'
        QtMocHelpers::MethodData<bool()>(9, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'launchAny'
        QtMocHelpers::MethodData<bool()>(10, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'hasClash'
        QtMocHelpers::MethodData<bool()>(11, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'hasV2ray'
        QtMocHelpers::MethodData<bool()>(12, 2, QMC::AccessPublic, QMetaType::Bool),
        // Method 'copyToClipboard'
        QtMocHelpers::MethodData<void(const QString &)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 14 },
        }}),
        // Method 'quickStatus'
        QtMocHelpers::MethodData<QString()>(15, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'statusAsync'
        QtMocHelpers::MethodData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'clashExe'
        QtMocHelpers::MethodData<QString()>(17, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'v2rayExe'
        QtMocHelpers::MethodData<QString()>(18, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ProxyService, qt_meta_tag_ZN12ProxyServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ProxyService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ProxyServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ProxyServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12ProxyServiceE_t>.metaTypes,
    nullptr
} };

void ProxyService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ProxyService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->statusReady((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: { bool _r = _t->isClashRunning();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 2: { bool _r = _t->isV2rayRunning();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { bool _r = _t->isClashPortOpen();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 4: { bool _r = _t->isV2rayPortOpen();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->launchClash();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->launchV2ray();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->launchAny();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->hasClash();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->hasV2ray();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 10: _t->copyToClipboard((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: { QString _r = _t->quickStatus();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->statusAsync(); break;
        case 13: { QString _r = _t->clashExe();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->v2rayExe();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ProxyService::*)(QString )>(_a, &ProxyService::statusReady, 0))
            return;
    }
}

const QMetaObject *ProxyService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProxyService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12ProxyServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ProxyService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void ProxyService::statusReady(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
