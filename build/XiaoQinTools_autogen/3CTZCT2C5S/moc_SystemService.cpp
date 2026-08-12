/****************************************************************************
** Meta object code from reading C++ file 'SystemService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Services/SystemService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SystemService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13SystemServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto SystemService::qt_create_metaobjectdata<qt_meta_tag_ZN13SystemServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SystemService",
        "cleanupDone",
        "",
        "message",
        "cleanJunk",
        "cleanMemory",
        "listStartupItems",
        "startupItemList",
        "disableStartupItem",
        "name",
        "enableStartupItem",
        "cleanJunkAsync",
        "scanLargeFiles",
        "clipboardText"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'cleanupDone'
        QtMocHelpers::SignalData<void(QString)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'cleanJunk'
        QtMocHelpers::MethodData<QString()>(4, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'cleanMemory'
        QtMocHelpers::MethodData<QString()>(5, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'listStartupItems'
        QtMocHelpers::MethodData<QString()>(6, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'startupItemList'
        QtMocHelpers::MethodData<QStringList()>(7, 2, QMC::AccessPublic, QMetaType::QStringList),
        // Method 'disableStartupItem'
        QtMocHelpers::MethodData<bool(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 9 },
        }}),
        // Method 'enableStartupItem'
        QtMocHelpers::MethodData<bool(const QString &)>(10, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 9 },
        }}),
        // Method 'cleanJunkAsync'
        QtMocHelpers::MethodData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'scanLargeFiles'
        QtMocHelpers::MethodData<QString()>(12, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'clipboardText'
        QtMocHelpers::MethodData<QString()>(13, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SystemService, qt_meta_tag_ZN13SystemServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SystemService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13SystemServiceE_t>.metaTypes,
    nullptr
} };

void SystemService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SystemService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->cleanupDone((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: { QString _r = _t->cleanJunk();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 2: { QString _r = _t->cleanMemory();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 3: { QString _r = _t->listStartupItems();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 4: { QStringList _r = _t->startupItemList();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 5: { bool _r = _t->disableStartupItem((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->enableStartupItem((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->cleanJunkAsync(); break;
        case 8: { QString _r = _t->scanLargeFiles();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 9: { QString _r = _t->clipboardText();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SystemService::*)(QString )>(_a, &SystemService::cleanupDone, 0))
            return;
    }
}

const QMetaObject *SystemService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13SystemServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SystemService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void SystemService::cleanupDone(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}
QT_WARNING_POP
