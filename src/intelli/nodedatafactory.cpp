/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodedatafactory.h"
#include "intelli/nodedata.h"
#include "intelli/node/dummy.h"

#include "gt_qtutilities.h"
#include "gt_logging.h"

using namespace intelli;

namespace
{

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

struct NodeDataFactory::Impl
{
    /// registered type names (used as default port captions)
    QHash<TypeId, TypeName> typeNames;
    /// registered conversion functions
    QMultiHash<TypeId, Conversion> conversions;
};

NodeDataFactory::NodeDataFactory() :
    pimpl(std::make_unique<Impl>())
{
    registerData(GT_METADATA(DummyData));
}

NodeDataFactory::~NodeDataFactory() = default;

NodeDataFactory&
NodeDataFactory::instance()
{
    static NodeDataFactory self;
    return self;
}

bool
NodeDataFactory::registerData(const QMetaObject& meta) noexcept
{
    QString className = meta.className();

    gtTrace().verbose().nospace()
        << "### Registering Data '" << className << "'...";

    if (!meta.inherits(&NodeData::staticMetaObject))
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(not derived of intelli::NodeData)")
                        .arg(className);
        return false;
    }

    if (!registerClass(meta)) return false;

    auto obj = std::unique_ptr<GtObject>(newObject(className));
    auto tmp = gt::unique_qobject_cast<NodeData>(std::move(obj));
    if (!tmp)
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! "
                           "(not invokable?)")
                         .arg(className);
        unregisterClass(meta);
        return false;
    }

    QString const& typeName = tmp->typeName();
    if (typeName.isEmpty())
    {
        gtError() << QObject::tr("Failed to register node data '%1'! (invalid type name)")
                         .arg(className);
        unregisterClass(meta);
        return false;
    }

    pimpl->typeNames.insert(className, typeName);

    registerConversion(className, typeId<DummyData>(), [](NodeDataPtr const&){
        return nullptr;
    });
    registerConversion(typeId<DummyData>(), className, [](NodeDataPtr const&){
        return nullptr;
    });
    return true;
}

bool
NodeDataFactory::registerConversion(TypeId const& from,
                                    TypeId const& to,
                                    ConversionFunction conversion) noexcept
{
    if (from.isEmpty() || to.isEmpty() || !conversion) return false;

    gtTrace().verbose().nospace()
        << "### Registering Conversion from '"<< from << "' to '" << to << "'...";

    pimpl->conversions.insert(from, {to, conversion});
    return true;
}

TypeName const&
NodeDataFactory::typeName(TypeId const& typeId) const noexcept
{
    static QString dummy{};

    auto iter = pimpl->typeNames.find(typeId);
    if (iter == pimpl->typeNames.end()) return dummy;
    return iter.value();
}

bool
NodeDataFactory::canConvert(TypeId const& from, TypeId const& to) const
{
    return from == to ||
           findConversion(pimpl->conversions, from, to) != pimpl->conversions.end();
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

    TypeId const& from = data->typeId();
    if (data->typeId() == to) return data;

    auto iter = findConversion(pimpl->conversions, from, to);
    if (iter == pimpl->conversions.end()) return nullptr;

    gtTrace().verbose()
        << QObject::tr("converting data from '%1' to '%2'...")
               .arg(from, to);

    return iter->convert(data);
}

NodeDataPtr
NodeDataFactory::makeData(TypeId const& typeId) const noexcept
{
    std::unique_ptr<GtObject> obj{
        const_cast<NodeDataFactory*>(this)->newObject(typeId)
    };

    return gt::unique_qobject_cast<NodeData>(std::move(obj));
}

