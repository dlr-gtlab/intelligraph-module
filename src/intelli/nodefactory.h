/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEFACTORY_H
#define GT_INTELLI_NODEFACTORY_H

#include <intelli/exports.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>

/// Helper macro for registering a node class. The node class does should not be
/// listed as a "data" object of your module. Use an empty string to "hide"
/// the node in the viewer.
#define GT_INTELLI_REGISTER_NODE(CLASS, CAT) \
    intelli::NodeFactory::registerNode<CLASS>(CAT);

namespace intelli
{

class Node;
class GT_INTELLI_EXPORT NodeFactory : public GtAbstractObjectFactory
{

public:

    ~NodeFactory();

    /**
     * @brief instance
     * @return
     */
    static NodeFactory& instance();

    QStringList registeredNodes() const;

    QStringList registeredCategories() const;

    QString nodeCategory(QString const& className) const noexcept;

    QString nodeModelName(QString const& className) const noexcept;

    /**
     * @brief Regsiters the node so that it can be used in intelli graphs
     * @param meta Metaobject of the node
     * @param category Category to list node in. Use an empty string to "hide"
     * the node in the viewer.
     * @return Success
     */
    bool registerNode(QMetaObject const& meta, QString category) noexcept;

    template <typename T>
    static bool registerNode(QString const& category)
    {
        return instance().registerNode(T::staticMetaObject, category);
    }

    /**
     * @brief Instantiates a new node of type className. May throw a
     * `logic_error` if the function fails.
     * @param className Class to instantiate
     * @return Object pointer (never null)
     */
    std::unique_ptr<Node> makeNode(QString const& className) const noexcept(false);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

    // hide some functions
    using GtAbstractObjectFactory::newObject;
    using GtAbstractObjectFactory::registerClass;

    NodeFactory();
};

} // namespace intelli

#endif // GT_INTELLI_NODEFACTORY_H
