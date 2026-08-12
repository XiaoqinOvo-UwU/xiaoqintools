/****************************************************************************
** Meta object code from reading C++ file 'UpdateService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Services/UpdateService.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UpdateService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN13UpdateServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto UpdateService::qt_create_metaobjectdata<qt_meta_tag_ZN13UpdateServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "UpdateService",
        "updateAvailableChanged",
        "",
        "checkFinished",
        "available",
        "downloadFinished",
        "ok",
        "message",
        "downloadStateChanged",
        "checkForUpdates",
        "downloadAndInstall",
        "currentVersion",
        "updateAvailable",
        "latestVersion",
        "downloadUrl",
        "downloading",
        "downloadProgress"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'updateAvailableChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'checkFinished'
        QtMocHelpers::SignalData<void(bool)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 4 },
        }}),
        // Signal 'downloadFinished'
        QtMocHelpers::SignalData<void(bool, QString)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 6 }, { QMetaType::QString, 7 },
        }}),
        // Signal 'downloadStateChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'checkForUpdates'
        QtMocHelpers::MethodData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'downloadAndInstall'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'currentVersion'
        QtMocHelpers::MethodData<QString()>(11, 2, QMC::AccessPublic, QMetaType::QString),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'updateAvailable'
        QtMocHelpers::PropertyData<bool>(12, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'latestVersion'
        QtMocHelpers::PropertyData<QString>(13, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'downloadUrl'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'downloading'
        QtMocHelpers::PropertyData<bool>(15, QMetaType::Bool, QMC::DefaultPropertyFlags, 3),
        // property 'downloadProgress'
        QtMocHelpers::PropertyData<int>(16, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<UpdateService, qt_meta_tag_ZN13UpdateServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject UpdateService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13UpdateServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13UpdateServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13UpdateServiceE_t>.metaTypes,
    nullptr
} };

void UpdateService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UpdateService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->updateAvailableChanged(); break;
        case 1: _t->checkFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->downloadFinished((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->downloadStateChanged(); break;
        case 4: _t->checkForUpdates(); break;
        case 5: _t->downloadAndInstall(); break;
        case 6: { QString _r = _t->currentVersion();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (UpdateService::*)()>(_a, &UpdateService::updateAvailableChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (UpdateService::*)(bool )>(_a, &UpdateService::checkFinished, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (UpdateService::*)(bool , QString )>(_a, &UpdateService::downloadFinished, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (UpdateService::*)()>(_a, &UpdateService::downloadStateChanged, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->updateAvailable(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->latestVersion(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->downloadUrl(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->downloading(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->downloadProgress(); break;
        default: break;
        }
    }
}

const QMetaObject *UpdateService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UpdateService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13UpdateServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UpdateService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void UpdateService::updateAvailableChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UpdateService::checkFinished(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void UpdateService::downloadFinished(bool _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void UpdateService::downloadStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
