/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_intelligraphdatafactory.h"
#include "gt_ignodedata.h"

#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_logging.h"

GtIntelliGraphDataFactory::GtIntelliGraphDataFactory() = default;

GtIntelliGraphDataFactory&
GtIntelliGraphDataFactory::instance()
{
    static GtIntelliGraphDataFactory self;
    return self;
}

bool
GtIntelliGraphDataFactory::registerData(const QMetaObject& meta) noexcept
{
    QString className = meta.className();

    gtTrace().nospace()
        << "### Registering Data '" << className << "'...";

    if (!registerClass(meta)) return false;

    auto obj = std::unique_ptr<GtObject>(newObject(className));
    auto tmp = gt::unique_qobject_cast<GtIgNodeData>(std::move(obj));
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
GtIntelliGraphDataFactory::typeName(const QString& className) const noexcept
{
    return m_typeNames.value(className);
}

std::unique_ptr<GtIgNodeData>
GtIntelliGraphDataFactory::newData(const QString& className) const noexcept
{
    std::unique_ptr<GtObject> obj{
        const_cast<GtIntelliGraphDataFactory*>(this)->newObject(className)
    };

    return gt::unique_qobject_cast<GtIgNodeData>(std::move(obj));
}

