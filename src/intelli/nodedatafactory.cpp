/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/nodedatafactory.h"
#include "intelli/nodedata.h"

#include "gt_qtutilities.h"
#include "gt_logging.h"

using namespace intelli;

NodeDataFactory::NodeDataFactory() = default;

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

    gtTrace().nospace()
        << "### Registering Data '" << className << "'...";

    if (!registerClass(meta)) return false;

    auto obj = std::unique_ptr<GtObject>(newObject(className));
    auto tmp = gt::unique_qobject_cast<NodeData>(std::move(obj));
    if (!tmp)
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! (not invokable?)")
               .arg(className);
        unregisterClass(meta);
        return false;
    }

    auto typeName = tmp->typeName();
    if (typeName.isEmpty())
    {
        gtError()
            << QObject::tr("Failed to register node data '%1'! (invalid type name)")
               .arg(className);
        unregisterClass(meta);
        return false;
    }

    m_typeNames.insert(className, typeName);
    return true;
}

QString
NodeDataFactory::typeName(const QString& typeId) const noexcept
{
    return m_typeNames.value(typeId);
}

std::unique_ptr<NodeData>
NodeDataFactory::newData(const QString& typeId) const noexcept
{
    return makeData(typeId);
}

std::unique_ptr<NodeData>
NodeDataFactory::makeData(const QString& typeId) const noexcept
{
    std::unique_ptr<GtObject> obj{
        const_cast<NodeDataFactory*>(this)->newObject(typeId)
    };

    return gt::unique_qobject_cast<NodeData>(std::move(obj));
}

