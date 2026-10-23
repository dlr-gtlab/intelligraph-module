/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodedatafactory.h"
#include "intelli/data/invalid.h"
#include "intelli/data/stringlist.h"

#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_logging.h"
#include "gt_coreapplication.h"

using namespace intelli;

namespace
{

inline QString
generateListType(QStringView const& typeId)
{
    return QStringLiteral("#list#") + typeId.toString();
}

/// Struct to store a conversion between two types.
struct Conversion
{
    /// target type id
    QString targetTypeId;
    /// conversion function
    ConversionFunction convert = nullptr;
};

inline auto
findConversion(QMultiHash<TypeId, Conversion> const& hash,
               TypeId const& from,
               TypeId const& to)
{
    auto iter = hash.find(from);

    while (iter != hash.end() && iter.key() == from)
    {
        if (iter->targetTypeId == to) return iter;
        ++iter;
    }

    return hash.end();
}

} // namespace

struct Entry
{
    /// registered meta objects
    // cppcheck-suppress unusedStructMember
    QMetaObject const* metaObject;

    /// module id associated with the registered class
    QString moduleId;

    /// registered type name (used for port captions)
    TypeName typeName;
};

struct ListEntry
{
    /// registered meta objects
    // cppcheck-suppress unusedStructMember
    QMetaObject const* metaObject;

    /// registered scalar type
    TypeId scalar;
};

struct NodeDataFactory::Impl
{
    /// registered types
    QHash<TypeId, Entry> types;
    /// registered list types
    QHash<TypeId, ListEntry> listTypes;
    /// registered conversion functions
    QMultiHash<TypeId, Conversion> conversions;
};

NodeDataFactory::NodeDataFactory() :
    pimpl(std::make_unique<Impl>())
{
    registerData(GT_METADATA(InvalidData), GT_MODULENAME());

    // for seamless backwards compatibility
    pimpl->listTypes.insert(GT_CLASSNAME(StringListData),
                            ListEntry{&StringListData::staticMetaObject, typeId<StringData>()});
}

NodeDataFactory::~NodeDataFactory() = default;

NodeDataFactory&
NodeDataFactory::instance()
{
    static NodeDataFactory self;
    return self;
}

bool
NodeDataFactory::registerData(QMetaObject const& meta, QString const& moduleId)
{
    QString className = meta.className();

    gtTrace().verbose().nospace()
        << "### Registering Data '" << className << "' (Module: " << moduleId << ")...";

    if (!meta.inherits(&NodeData::staticMetaObject))
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(not derived of intelli::NodeData)")
                        .arg(className);
        return false;
    }

    if (pimpl->types.contains(className))
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(duplicate entry)")
                   .arg(className);
        return false;
    }

    auto iter = pimpl->types.insert(className, Entry{&meta, moduleId, TypeId{}});

    auto removeOnFailure = gt::finally([&className, this](){
        bool success = pimpl->types.remove(className);
        assert(success);
    });

    auto tmp = makeData(className);
    if (!tmp)
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(not default-invokable?)")
                   .arg(className);
        return false;
    }

    iter->typeName = tmp->typeName();
    if (iter->typeName.isEmpty())
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(invalid type name)")
                   .arg(className);
        return false;
    }

    removeOnFailure.clear();

    // register conversion for invalid data type
    registerConversion(className, typeId<InvalidData>(), [](NodeDataPtr const&){
        return nullptr;
    });
    registerConversion(typeId<InvalidData>(), className, [](NodeDataPtr const&){
        return nullptr;
    });

    return true;
}

bool
NodeDataFactory::registerListType(TypeId typeId, QMetaObject const& meta)
{
    QString className = meta.className();

    gtTrace().verbose().nospace()
        << "### Registering List Data '" << className << "' for '" << typeId << "'...";

    if (!meta.inherits(&ListData::staticMetaObject))
    {
        gtError()
            << QObject::tr("Failed to register data list type '%1'! "
                           "(not derived of intelli::NodeData)")
                   .arg(className);
        return false;
    }

    QString listTypeId = generateListType(typeId);
    if (pimpl->listTypes.contains(listTypeId))
    {
        gtError()
            << QObject::tr("Failed to register data list type '%1'! "
                           "(duplicate entry)")
                   .arg(className);
        return false;
    }

    pimpl->listTypes.insert(listTypeId, ListEntry{&meta, typeId});

    // register conversion for invalid data type
    registerConversion(listTypeId, intelli::typeId<InvalidData>(), [](NodeDataPtr const&){
        return nullptr;
    });
    registerConversion(intelli::typeId<InvalidData>(), listTypeId, [](NodeDataPtr const&){
        return nullptr;
    });

    return true;
}

void
NodeDataFactory::unregisterData(QMetaObject const& meta)
{
    QString className = meta.className();

    auto entry = pimpl->types.constFind(className);
    if (entry == pimpl->types.cend()) return;

    gtTrace().verbose().nospace()
        << "### Unregistering Data '" << className << ")...";

    pimpl->types.erase(entry);

    unregisterConversions(className);

    QString listTypeId = generateListType(className);

    auto listEntry = pimpl->listTypes.constFind(listTypeId);
    if (listEntry == pimpl->listTypes.cend()) return;

    pimpl->listTypes.erase(listEntry);

    unregisterConversions(listTypeId);
}

bool
NodeDataFactory::registerConversion(TypeId const& from,
                                    TypeId const& to,
                                    ConversionFunction conversion)
{
    gtTrace().verbose().nospace()
        << "### Registering Conversion from '"<< from << "' to '" << to << "'...";

    if (!isKnownType(from))
    {
        gtError()
            << QObject::tr("Failed to register conversion from '%1' to '%2'! "
                           "(Unknown type '%1')")
                   .arg(from, to);
        return false;
    }
    if (!isKnownType(to))
    {
        gtError()
            << QObject::tr("Failed to register conversion from '%1' to '%2'! "
                           "(Unknown type '%2')")
                   .arg(from, to);
        return false;
    }
    if (!conversion)
    {
        gtError()
            << QObject::tr("Failed to register conversion from '%1' to '%2'! "
                           "(Invalid conversion)")
                   .arg(from, to);
        return false;
    }

    if (findConversion(pimpl->conversions, from, to) != pimpl->conversions.cend())
    {
        gtError()
            << QObject::tr("Failed to register conversion from '%1' to '%2'! "
                           "(Duplicate conversion)")
                   .arg(from, to);
        return false;
    }

    pimpl->conversions.insert(from, {to, conversion});
    return true;
}

void
NodeDataFactory::unregisterConversions(TypeId const& type)
{
    auto entry = pimpl->conversions.constFind(type);
    if (entry != pimpl->conversions.cend())
    {
        pimpl->conversions.erase(entry);
    }

    for (TypeId const& from : pimpl->conversions.uniqueKeys())
    {
        assert(from != type); // we have removed this one
        unregisterConversion(from, type);
    }
}

void
NodeDataFactory::unregisterConversion(TypeId const& from, TypeId const& to)
{
    auto conv = findConversion(pimpl->conversions, from, to);
    if (conv != pimpl->conversions.cend())
    {
        pimpl->conversions.erase(conv);
    }
}

QMetaObject const*
NodeDataFactory::findMetaObject(TypeId const& anyTypeId) const noexcept
{
    auto iter = pimpl->types.find(anyTypeId);
    if (iter != pimpl->types.end())
    {
        return iter->metaObject;
    }

    auto listIter = pimpl->listTypes.find(anyTypeId);
    if (listIter != pimpl->listTypes.end())
    {
        return listIter->metaObject;
    }
    return nullptr;
}

bool
NodeDataFactory::isKnownType(TypeId const& typeId) const
{
    return (pimpl->types.contains(typeId) || isListType(typeId));
}

TypeIdList
intelli::NodeDataFactory::registeredTypeIds() const
{
    TypeIdList types;
    std::copy(pimpl->types.keyBegin(), pimpl->types.keyEnd(), std::back_inserter(types));
    types.removeOne(typeId<InvalidData>());
    return types;
}

TypeIdList
NodeDataFactory::validTypeIds() const
{
    return registeredTypeIds();
}

TypeName
NodeDataFactory::typeName(TypeId const& typeId) const
{
    if (isListType(typeId))
    {
        return QStringLiteral("%1_list").arg(typeName(innerType(typeId)));
    }

    auto iter = pimpl->types.constFind(typeId);
    if (iter == pimpl->types.cend()) return {};
    return iter->typeName;
}

bool
NodeDataFactory::isListType(TypeId const& typeId) const
{
    // emit warning if accessing GT_CLASSNAME(StringListData)
    if (typeId == QString{GT_CLASSNAME(StringListData)} && gtApp && gtApp->devMode())
    {
        gtLogOnce(Warning)
            << QObject::tr("use intelli::typeId<T>() instead of GT_CLASSNAME(T)!");
    }
    return pimpl->listTypes.contains(typeId);
}

bool
NodeDataFactory::hasListType(TypeId const& typeId) const
{
    if (isListType(typeId)) return false;
    if (typeId == intelli::typeId<InvalidData>()) return false;

    return pimpl->listTypes.contains(generateListType(typeId));
}

TypeId
NodeDataFactory::innerType(TypeId const& typeId) const
{
    auto iter = pimpl->listTypes.constFind(typeId);
    if (iter == pimpl->listTypes.cend()) return {};
    return iter->scalar;
}

TypeId
NodeDataFactory::listType(TypeId const& typeId) const
{
    if (!hasListType(typeId)) return {};

    return generateListType(typeId);
}

bool
NodeDataFactory::canConvert(TypeId const& from, TypeId const& to) const
{
    return from == to ||
           findConversion(pimpl->conversions, from, to) != pimpl->conversions.cend();
}

bool
NodeDataFactory::canConvert(TypeId const& a, TypeId const& b, PortType direction) const
{
    return (direction == PortType::Out) ? canConvert(a, b) : canConvert(b, a);
}

NodeDataPtr
NodeDataFactory::convert(NodeDataPtr const& data, TypeId const& to) const
{
    if (!data) return nullptr;

    QMetaObject const* srcMetaObject =  data->metaObject();
    if (!srcMetaObject) return nullptr;

    QMetaObject const* targetMetaObject = findMetaObject(to);
    if (!targetMetaObject) return nullptr;

    if (srcMetaObject->inherits(targetMetaObject)) return data;

    TypeId from = data->typeId();

    auto iter = findConversion(pimpl->conversions, from, to);
    if (iter == pimpl->conversions.cend()) return nullptr;

    gtTrace().verbose()
        << QObject::tr("converting data from '%1' to '%2'...").arg(from, to);

    return iter->convert(data);
}

std::unique_ptr<NodeData>
NodeDataFactory::makeData(TypeId const& typeId) const
{
    if (isListType(typeId)) return makeListData(typeId);

    auto iter = pimpl->types.constFind(typeId);
    if (iter == pimpl->types.cend())
    {
        gtError() << QObject::tr("Failed to instantiate NodeData '%1'! "
                                 "(unknown type)").arg(typeId);
        return {};
    }

    auto object = std::unique_ptr<QObject>(iter->metaObject->newInstance());
    if (!object)
    {
        gtError() << QObject::tr("Failed to instantiate NodeData '%1'! "
                                 "(not invokable?)").arg(typeId);
        return {};
    }

    return gt::unique_qobject_cast<NodeData>(std::move(object));
}

std::unique_ptr<ListData>
NodeDataFactory::makeListData(TypeId const& typeId) const
{
    auto entry = pimpl->listTypes.constFind(isListType(typeId) ?
                                                typeId : listType(typeId));
    if (entry == pimpl->listTypes.cend())
    {
        return nullptr;
    }

    std::unique_ptr<QObject> rawListData{entry->metaObject->newInstance()};
    if (!rawListData) return nullptr;

    return gt::unique_qobject_cast<ListData>(std::move(rawListData));
}
