/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GTINTELLIGRAPHNODEFACTORY_H
#define GTINTELLIGRAPHNODEFACTORY_H

#include "gt_abstractobjectfactory.h"

#include "gt_object.h"
#include "gt_intelligraph_exports.h"

#include <QtNodes/NodeDelegateModelRegistry>

/// Helper macro for registering a node class. The node class does should not be
/// listed as a "data" object of your module
#define GTIG_REGISTER_NODE(CLASS, CAT) \
    struct RegisterNodeOnce ## CLASS { \
        RegisterNodeOnce ## CLASS() { \
            GtIntelliGraphNodeFactory::instance() \
                .registerNode(GT_METADATA(CLASS), CAT); \
        } \
    }; \
    static RegisterNodeOnce ## CLASS s_register_node_once_##CLASS;

class GtIntelliGraphNode;
class GT_IG_EXPORT GtIntelliGraphNodeFactory : public GtAbstractObjectFactory
{
    using NodeDelegateModelRegistry = QtNodes::NodeDelegateModelRegistry;
public:

    GtIntelliGraphNodeFactory();

    /**
     * @brief instance
     * @return
     */
    static GtIntelliGraphNodeFactory& instance();

    bool registerNode(QMetaObject const& meta, QString const& category);

    std::unique_ptr<GtIntelliGraphNode> newNode(QString const& className
                                                ) noexcept(false);

    std::unique_ptr<NodeDelegateModelRegistry> makeRegistry();

private:

    // hide some functions
    using GtAbstractObjectFactory::newObject;

    using ClassName = QString;
    using NodeCategory = QString;

    QHash<ClassName, NodeCategory> m_categories;

    /// hide default register class method
    bool registerClass(QMetaObject metaObj) final;
};

#endif // GTINTELLIGRAPHNODEFACTORY_H
