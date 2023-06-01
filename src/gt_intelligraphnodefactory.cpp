/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "gt_intelligraphnodefactory.h"

#include "gt_intelligraphnode.h"
#include "gt_objectfactory.h"
#include "gt_qtutilities.h"
#include "models/gt_intelligraphobjectmodel.h"

GtIntelliGraphNodeFactory::GtIntelliGraphNodeFactory()
{

}

GtIntelliGraphNodeFactory&
GtIntelliGraphNodeFactory::instance()
{
    static GtIntelliGraphNodeFactory self;
    return self;
}

bool
GtIntelliGraphNodeFactory::registerClass(QMetaObject metaObj)
{
    return GtAbstractObjectFactory::registerClass(metaObj);
}

bool
GtIntelliGraphNodeFactory::registerNode(QMetaObject const& meta,
                                        QString const& category)
{
    QString className = meta.className();

    gtTrace().nospace()
        << "### Registering Node '" << className
        << "' (Category: " << category << ")...";

    if (!registerClass(meta)) return false;

    // add node to object factory
    if (!gtObjectFactory->knownClass(className) &&
        !gtObjectFactory->registerClass(meta))
    {
        gtError() << QObject::tr("Failed to register node in object factory!");
        return false;
    }

    m_categories.insert(className, category);
    return true;
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

std::unique_ptr<GtIntelliGraphNodeFactory::NodeDelegateModelRegistry>
GtIntelliGraphNodeFactory::makeRegistry()
{
    auto registry = std::make_unique<QtNodes::NodeDelegateModelRegistry>();

    for (auto const& name : knownClasses())
    {
        auto cat = m_categories.value(name);
        if (cat.isEmpty()) cat = QStringLiteral("Unknown");

        registry->registerModel<GtIntelliGraphObjectModel>([=](){
            return std::make_unique<GtIntelliGraphObjectModel>(name);
        }, cat);
    }

    return registry;
}
