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
#include <intelli/globals.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>

/// Helper macro for registering a node class. The node class does should not be
/// registered additionally as a "data" object of your module
#define GT_INTELLI_REGISTER_DATA(CLASS) \
    intelli::NodeDataFactory::registerData<CLASS>();

namespace intelli
{

class NodeData;
class GT_INTELLI_EXPORT NodeDataFactory : public GtAbstractObjectFactory
{

public:

    /**
     * @brief instance
     * @return
     */
    static NodeDataFactory& instance();

    bool registerData(QMetaObject const& meta) noexcept;

    template <typename T>
    static bool registerData()
    {
        return instance().registerData(T::staticMetaObject);
    }

    TypeIdList registeredTypeIds() const { return knownClasses(); };

    TypeName typeName(TypeId const& typeId) const noexcept;

    bool canConvert(TypeId const& a, TypeId const& b) const;

    /**
     * @brief Instantiates a new node of type className.
     * @param className Class to instantiate
     * @return Object pointer (may be null)
     */
    std::unique_ptr<NodeData> makeData(TypeId const& typeId) const noexcept;

private:

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    QHash<TypeId, TypeName> m_typeNames;

    NodeDataFactory();
};

} // namespace intelli

#endif // GT_INTELLI_DATAFACTORY_H
