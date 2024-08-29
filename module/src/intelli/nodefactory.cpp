/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 6.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/nodefactory.h"

#include "intelli/node.h"
//#include "intelli/adapter/objectmodel.h"

#include "gt_objectfactory.h"
#include "gt_utilities.h"
#include "gt_qtutilities.h"
#include "gt_algorithms.h"

using namespace intelli;

NodeFactory::NodeFactory() = default;

NodeFactory&
NodeFactory::instance()
{
    static NodeFactory self;
    return self;
}

QStringList
NodeFactory::registeredCategories() const
{
    QStringList categories;

    for (NodeMetaData const& data : m_data)
    {
        if (!categories.contains(data.category)) categories << data.category;
    }

    return categories;
}

QString
NodeFactory::nodeCategory(const QString& className) const noexcept
{
    return m_data.value(className).category;
}

QString
NodeFactory::nodeModelName(const QString& className) const noexcept
{
    return m_data.value(className).modelName;
}

bool
NodeFactory::registerNode(QMetaObject const& meta,
                          QString const& category) noexcept
{
    QString className = meta.className();

    gtTrace().verbose().nospace()
        << "### Registering Node '" << className
        << "' (Category: " << category << ")...";

    if (!registerClass(meta)) return false;

    // add node to object factory
    if (!gtObjectFactory->knownClass(className) &&
        !gtObjectFactory->registerClass(meta))
    {
        gtError() << QObject::tr("Failed to register node '%1' in object factory!")
                         .arg(className);
        return false;
    }

    auto obj = std::unique_ptr<GtObject>(newObject(className));
    auto tmp = gt::unique_qobject_cast<Node>(std::move(obj));
    if (!tmp)
    {
        gtError()  << QObject::tr("Failed to register node '%1'! (not invokable?)")
                          .arg(className);
        unregisterClass(meta);
        return false;
    }

    m_data.insert(className, {category, tmp->modelName()});
    return true;
}

std::unique_ptr<Node>
NodeFactory::makeNode(QString const& className) const noexcept(false)
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

    node->setActive(true);

    return node;
}
