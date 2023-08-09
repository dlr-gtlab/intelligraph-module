/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_DATAFACTORY_H
#define GT_INTELLI_DATAFACTORY_H

#include <intelli/exports.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>

/// Helper macro for registering a node class. The node class does should not be
/// registered additionally as a "data" object of your module
#define GTIG_REGISTER_DATA(DATA) \
struct RegisterDataOnce ## DATA { \
        RegisterDataOnce ## DATA() { \
            intelli::NodeDataFactory::instance() \
                .registerData(GT_METADATA(DATA)); \
    } \
}; \
    static RegisterDataOnce ## DATA s_register_data_once_##DATA;

namespace intelli
{

class NodeData;
class GT_INTELLI_EXPORT NodeDataFactory : public GtAbstractObjectFactory
{

public:

    NodeDataFactory();

    /**
     * @brief instance
     * @return
     */
    static NodeDataFactory& instance();

    bool registerData(QMetaObject const& meta) noexcept;

    QStringList registeredTypeIds() const { return knownClasses(); };

    QString typeName(QString const& typeId) const noexcept;

    std::unique_ptr<NodeData> newData(QString const& typeId) const noexcept;

private:

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    using TypeId   = QString;
    using TypeName = QString;

    QHash<TypeId, TypeName> m_typeNames;
};

} // namespace intelli

using GtIntelliGraphDataFactory [[deprecated]] = intelli::NodeDataFactory;

#endif // GT_INTELLI_DATAFACTORY_H
