/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnodefactory.h"

#include "gt_intelligraphnode.h"

GtIntelliGraphNodeFactory::GtIntelliGraphNodeFactory()
{

}

GtIntelliGraphNodeFactory&
GtIntelliGraphNodeFactory::instance()
{
    static GtIntelliGraphNodeFactory self;
    return self;
}

std::unique_ptr<GtIntelliGraphNode>
GtIntelliGraphNodeFactory::newNode(QString const& className) noexcept(false)
{
    std::unique_ptr<GtObject> obj{newObject(className)};

    auto node = gt::unique_qobject_cast<GtIntelliGraphNode>(std::move(obj));

    if (!node)
    {
        std::logic_error e{
            std::string{
                "Failed to create node for classname: " +
                gt::squoted(className.toStdString())
            }
        };
        gtError() << e.what();
        gtError() << QObject::tr("Object may not be invokable. Known classes:")
                  << knownClasses();
        throw e;
    }

    return node;
}
