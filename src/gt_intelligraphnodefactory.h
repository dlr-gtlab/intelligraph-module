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

#define GTIG_REGISTER_NODE(CLASS) \
    struct RegisterNodeOnce ## CLASS { \
        RegisterNodeOnce ## CLASS() { \
            gtTrace().nospace() << "### Registering Node '" << GT_CLASSNAME(CLASS) << "'"; \
            GtIntelliGraphNodeFactory::instance().registerClass(GT_METADATA(CLASS)); \
        } \
    }; \
    static RegisterNodeOnce ## CLASS s_register_node_once_##CLASS;

class GtIntelliGraphNode;
class GtIntelliGraphNodeFactory : public GtAbstractObjectFactory
{
public:

    GtIntelliGraphNodeFactory();

    /**
     * @brief instance
     * @return
     */
    static GtIntelliGraphNodeFactory& instance();

    std::unique_ptr<GtIntelliGraphNode> newNode(QString const& className
                                                ) noexcept(false);

private:
    // hide some functions
    using GtAbstractObjectFactory::newObject;
};

#endif // GTINTELLIGRAPHNODEFACTORY_H
