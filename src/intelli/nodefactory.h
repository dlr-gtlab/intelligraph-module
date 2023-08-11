/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_NODEFACTORY_H
#define GT_INTELLI_NODEFACTORY_H

#include <intelli/exports.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>

#define GTIG_REGISTER_NODE(CLASS, CAT) \
    struct RegisterNodeOnce ## CLASS { \
        [[deprecated("Use GT_INTELLI_REGISTER_NODE instead")]] \
        RegisterNodeOnce ## CLASS() { \
            intelli::NodeFactory::instance() \
                .registerNode(GT_METADATA(CLASS), CAT); \
        } \
    }; \
    static RegisterNodeOnce ## CLASS s_register_node_once_##CLASS;

/// Helper macro for registering a node class. The node class does should not be
/// listed as a "data" object of your module. Use an empty string to "hide"
/// the node in the viewer.
#define GT_INTELLI_REGISTER_NODE(CLASS, CAT) \
    struct RegisterNodeOnce ## CLASS { \
        RegisterNodeOnce ## CLASS() { \
            intelli::NodeFactory::instance() \
                .registerNode(GT_METADATA(CLASS), CAT); \
        } \
    }; \
    static RegisterNodeOnce ## CLASS s_register_node_once_##CLASS;

namespace QtNodes { class NodeDelegateModelRegistry; }

namespace intelli
{

class Node;
class GT_INTELLI_EXPORT NodeFactory : public GtAbstractObjectFactory
{
    using NodeDelegateModelRegistry = QtNodes::NodeDelegateModelRegistry;

public:

    NodeFactory();

    /**
     * @brief instance
     * @return
     */
    static NodeFactory& instance();

    auto registeredNodes() const { return knownClasses(); };

    QString nodeCategory(QString const& className) const noexcept;

    /**
     * @brief Regsiters the node so that it can be used in intelli graphs
     * @param meta Metaobject of the node
     * @param category Category to list node in. Use an empty string to "hide"
     * the node in the viewer.
     * @return Success
     */
    bool registerNode(QMetaObject const& meta, QString const& category) noexcept;
    
    std::unique_ptr<Node> newNode(QString const& className
                                                ) const noexcept(false);

    std::unique_ptr<NodeDelegateModelRegistry> makeRegistry() noexcept;

private:

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    using ClassName = QString;
    using NodeCategory = QString;

    QHash<ClassName, NodeCategory> m_categories;
};

} // namespace intelli

using GtIntelliGraphNodeFactory [[deprecated]] = intelli::NodeFactory;

#endif // GT_INTELLI_NODEFACTORY_H
