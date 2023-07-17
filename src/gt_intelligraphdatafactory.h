/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GTINTELLIGRAPHDATAFACTORY_H
#define GTINTELLIGRAPHDATAFACTORY_H

#include "gt_intelligraph_exports.h"

#include "gt_abstractobjectfactory.h"
#include "gt_object.h"

/// Helper macro for registering a node class. The node class does should not be
/// registered additionally as a "data" object of your module
#define GTIG_REGISTER_DATA(DATA) \
struct RegisterDataOnce ## DATA { \
        RegisterDataOnce ## DATA() { \
            GtIntelliGraphDataFactory::instance() \
                .registerData(GT_METADATA(DATA)); \
    } \
}; \
    static RegisterDataOnce ## DATA s_register_data_once_##DATA;

class GtIgNodeData;
class GT_IG_EXPORT GtIntelliGraphDataFactory : public GtAbstractObjectFactory
{

public:

    GtIntelliGraphDataFactory();

    /**
     * @brief instance
     * @return
     */
    static GtIntelliGraphDataFactory& instance();

    bool registerData(QMetaObject const& meta) noexcept;

    QStringList registeredTypeIds() const { return knownClasses(); };

    QString typeName(QString const& typeId) const noexcept;

    std::unique_ptr<GtIgNodeData> newData(QString const& typeId) const noexcept;

private:

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    using TypeId   = QString;
    using TypeName = QString;

    QHash<TypeId, TypeName> m_typeNames;
};

#endif // GTINTELLIGRAPHDATAFACTORY_H
