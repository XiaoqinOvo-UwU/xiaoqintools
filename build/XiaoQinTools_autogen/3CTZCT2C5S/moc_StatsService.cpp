/****************************************************************************
** Meta object code from reading C++ file 'StatsService.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/Services/StatsService.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatsService.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12StatsServiceE_t {};
} // unnamed namespace

template <> constexpr inline auto StatsService::qt_create_metaobjectdata<qt_meta_tag_ZN12StatsServiceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "StatsService",
        "record",
        "",
        "type",
        "detail",
        "categoryCounts",
        "QVariantList",
        "moodCounts",
        "cleanDaily",
        "proxyCounts",
        "overviewStats"
    };

    QtMocHelpers::UintData qt_methods {
        // Method 'record'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 4 },
        }}),
        // Method 'categoryCounts'
        QtMocHelpers::MethodData<QVariantList()>(5, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'moodCounts'
        QtMocHelpers::MethodData<QVariantList()>(7, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'cleanDaily'
        QtMocHelpers::MethodData<QVariantList()>(8, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'proxyCounts'
        QtMocHelpers::MethodData<QVariantList()>(9, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'overviewStats'
        QtMocHelpers::MethodData<QVariantList()>(10, 2, QMC::AccessPublic, 0x80000000 | 6),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<StatsService, qt_meta_tag_ZN12StatsServiceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject StatsService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12StatsServiceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12StatsServiceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12StatsServiceE_t>.metaTypes,
    nullptr
} };

void StatsService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<StatsService *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->record((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: { QVariantList _r = _t->categoryCounts();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 2: { QVariantList _r = _t->moodCounts();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 3: { QVariantList _r = _t->cleanDaily();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 4: { QVariantList _r = _t->proxyCounts();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 5: { QVariantList _r = _t->overviewStats();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *StatsService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StatsService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12StatsServiceE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int StatsService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
