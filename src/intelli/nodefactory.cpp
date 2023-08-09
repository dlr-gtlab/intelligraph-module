/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/nodefactory.h"

#include "intelli/node.h"
#include "intelli/adapter/objectmodel.h"

#include "gt_objectfactory.h"
#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_algorithms.h"

#include <QtNodes/NodeDelegateModelRegistry>

using namespace intelli;

NodeFactory::NodeFactory() = default;

NodeFactory&
NodeFactory::instance()
{
    static NodeFactory self;
    return self;
}

QString
NodeFactory::nodeCategory(const QString& className) const noexcept
{
    return m_categories.value(className);
}

bool
NodeFactory::registerNode(QMetaObject const& meta,
                                        QString const& category) noexcept
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

std::unique_ptr<Node>
NodeFactory::newNode(QString const& className) const noexcept(false)
{
    std::unique_ptr<GtObject> obj{
                                  const_cast<NodeFactory*>(this)->newObject(className)
    };
    
    auto node = gt::unique_qobject_cast<Node>(std::move(obj));

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

std::unique_ptr<NodeFactory::NodeDelegateModelRegistry>
NodeFactory::makeRegistry() noexcept
{
    auto registry = std::make_unique<QtNodes::NodeDelegateModelRegistry>();

    gt::for_each_key(m_knownClasses.begin(), m_knownClasses.end(),
                     [&, this](auto const& name){
        auto const& cat = m_categories.value(name);

        registry->registerModel<GtIntelliGraphObjectModel>([=](){
            return std::make_unique<GtIntelliGraphObjectModel>(name);
        }, cat);
    });

    return registry;
}
