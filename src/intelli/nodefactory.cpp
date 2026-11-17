/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodefactory.h"

#include "intelli/node.h"

#include "gt_objectfactory.h"
#include "gt_utilities.h"
#include "gt_qtutilities.h"

#include <tuple>
#include <algorithm>

using namespace intelli;

const char* S_DEPRECATED = "[DEPRECATED]";

struct NodeFactory::Impl
{
    using ClassName = QString;
    using NodeCategory = QString;
    using NodeName = QString;

    struct NodeMetaData
    {
        QVector<NodeCategory> categories;
        NodeName modelName;
    };

    QHash<ClassName, NodeMetaData> data;
    QHash<NodeCategory, int> orderPriorities;
};

NodeFactory::NodeFactory() :
    impl(std::make_unique<Impl>())
{
    // deprecated category should be placed last
    pimpl()->orderPriorities[S_DEPRECATED] = CategoryOrderPriority::Last + 1;
}

NodeFactory::~NodeFactory() = default;

NodeFactory&
NodeFactory::instance() noexcept
{
    static NodeFactory self;
    return self;
}

NodeFactory::Impl*
NodeFactory::pimpl() noexcept { return impl.get(); }

NodeFactory::Impl const*
NodeFactory::pimpl() const noexcept { return impl.get(); }

QStringList
NodeFactory::registeredNodes() const
{
    return knownClasses();
}

QStringList
NodeFactory::registeredCategories() const
{
    QStringList categories;

    for (Impl::NodeMetaData const& data : pimpl()->data)
    {
        std::copy_if(data.categories.begin(),
                     data.categories.end(),
                     std::back_inserter(categories),
                     [&categories](QString const& cat){
            return !categories.contains(cat);
        });
    }

    return categories;
}

QStringList
NodeFactory::sortedCategories() const
{
    QStringList categories = registeredCategories();

    std::sort(categories.begin(), categories.end(),
              [this](QString const& a, QString const& b){
        // sort by order prio and then by name
        int oa = categoryOrderPriority(a);
        int ob = categoryOrderPriority(b);
        return std::tie(oa, a) < std::tie(ob, b);
    });

    return categories;
}

QString
NodeFactory::nodeCategory(QString const& className) const
{
    auto categories = nodeCategories(className);
    return categories.empty() ? QString{} : categories.front();
}

View<QString>
NodeFactory::nodeCategories(QString const& className) const
{
    auto const entry = pimpl()->data.find(className);
    if (entry == pimpl()->data.end()) return {};
    return entry->categories;
}

QString
NodeFactory::nodeModelName(QString const& className) const
{
    return pimpl()->data.value(className).modelName;
}

bool
NodeFactory::isKnownNode(QString const& className) const
{
    return knownClass(className);
}

bool
NodeFactory::registerNode(QMetaObject const& meta,
                          QString const& category,
                          QString moduleId)
{
    return registerNode(meta, View<QString>{category}, moduleId);
}

bool
NodeFactory::registerNode(QMetaObject const& meta,
                          View<QString> categories,
                          QString moduleId)
{
    QString const className = meta.className();

    gtTrace().verbose().nospace()
        << "### Registering Node '" << className
        << "' (Categories: " << gt::log::range(categories)
        << ", Module: " << moduleId << ")...";

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
    if (!registerClass(meta, moduleId)) return false;
#else
    if (!registerClass(meta)) return false;
#endif

    // add node to object factory
    if (!gtObjectFactory->knownClass(className) &&
#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
        !gtObjectFactory->registerClass(meta, moduleId))
#else
        !gtObjectFactory->registerClass(meta))
#endif
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

    Impl::NodeMetaData entry{
        QVector<QString>{categories.begin(), categories.end()}, tmp->modelName()
    };

    // remove duplicates
    std::sort(entry.categories.begin(), entry.categories.end());
    entry.categories.erase(std::unique(entry.categories.begin(),
                                       entry.categories.end()),
                           entry.categories.end());
    entry.categories.removeAll(QString{});

    if (tmp->nodeFlags() & NodeFlag::Deprecated && !entry.categories.empty())
    {
        entry.categories = {S_DEPRECATED};
    }

    pimpl()->data.insert(className, std::move(entry));
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
                gt::squoted(className.toStdString()) +
                " (object may not be invokable)"
            }
        };
        gtError() << e.what();
        throw e;
    }

    node->setActive(true);
    return node;
}

std::unique_ptr<Node>
NodeFactory::tryMakeNode(const QString& className) const
{
    std::unique_ptr<GtObject> obj{
        const_cast<NodeFactory*>(this)->newObject(className)
    };

    auto node = gt::unique_qobject_cast<Node>(std::move(obj));
    if (!node)
    {
        gtError() << QObject::tr("Failed to create node for class name '%1' "
                                 "(object may not be invokable)")
                         .arg(className);
        return nullptr;
    }

    node->setActive(true);
    return node;
}

void
NodeFactory::setCategoryOrderPriority(QString const& category, int value)
{
    if (category.isEmpty()) return;

    pimpl()->orderPriorities[category] = value;
}

int
NodeFactory::categoryOrderPriority(QString const& category) const
{
    return pimpl()->orderPriorities.value(category, CategoryOrderPriority::Normal);
}
