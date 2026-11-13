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
#include <intelli/view.h>

#include <gt_abstractobjectfactory.h>
#include <gt_object.h>
#include <gt_globals.h>

/// Helper macro for registering a node class. The node class does should not be
/// listed as a "data" object of your module. Use an empty string to "hide"
/// the node in the viewer.
#define GT_INTELLI_REGISTER_NODE(CLASS, ...) \
    intelli::NodeFactory::instance().registerNode<CLASS>(intelli::View<QString>{ __VA_ARGS__ });

namespace intelli
{

/// Helper struct for class-like enum. Denotes how categories should be ordered
struct CategoryOrderPriority
{
    enum Value : int
    {
        First  = -999,
        Normal = 0,
        Last   =  999
    };
};

class Node;
class GT_INTELLI_EXPORT NodeFactory : private GtAbstractObjectFactory
{

public:

    ~NodeFactory();

    /**
     * @brief instance
     * @return
     */
    static NodeFactory& instance() noexcept;

    /**
     * @brief Returns sorted list of registered nodes
     * @return Sorted list of nodes
     */
    QStringList registeredNodes() const;

    /**
     * @brief Returns sorted list of categories
     * @return Sorted list of categories
     */
    QStringList registeredCategories() const;

    [[deprecated("use `nodeCategories` instead")]]
    QString nodeCategory(QString const& className) const;

    /**
     * @brief Returns all associated categories of the given node
     * @param className Node's class name
     * @return Categories, sorted.
     */
    View<QString> nodeCategories(QString const& className) const;

    /**
     * @brief Returns the model name (= display name) of the given node
     * @param className Node's class name
     * @return Model name
     */
    QString nodeModelName(QString const& className) const;

    /**
     * @brief Registers the node so that it can be used by IntelliGraph
     * @param meta Metaobject of the node
     * @param category Category to list node in. Use an empty string to "hide"
     * the node in the scene menu.
     * @param moduleId Module Id of the associated node. Must not be empty.
     * @return Whether registration was successful
     */
    bool registerNode(QMetaObject const& meta, QString category, QString moduleId);

    /**
     * @brief View-based overload. Registers the node so that it can be used by
     * IntelliGraph
     * @param meta Metaobject of the node
     * @param categories Categories to list node in. Use an empty list to "hide"
     * the node in the scene menu.
     * @param moduleId Module Id of the associated node. Must not be empty.
     * @return Whether registration was successful
     */
    bool registerNode(QMetaObject const& meta, View<QString> categories, QString moduleId);

    /**
     * @brief Template based overload. Registers the Node `T`.
     * @tparam T Node
     * @param category Category to add node to
     * @return Whether registration was successful
     */
    template <typename T,
             typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    static inline bool registerNode(QString const& category)
    {
        return instance().registerNode(T::staticMetaObject, category, GT_MODULENAME());
    }

    /**
     * @brief Template based overload. Registers the Node `T`.
     * @tparam T Node
     * @param categories Categories to add node to
     * @return Whether registration was successful
     */
    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Node, T>::value>>
    inline bool registerNode(View<QString> categories)
    {
        return instance().registerNode(T::staticMetaObject, categories, GT_MODULENAME());
    }

    /**
     * @brief Instantiates a new node of type className. May throw a
     * `logic_error` if the function fails.
     * @param className Class to instantiate
     * @return Object pointer (never null)
     */
    std::unique_ptr<Node> makeNode(QString const& className) const noexcept(false);

    void setCategoryOrderPriority(QString const& category, int value);

    int categoryOrderPriority(QString const& category) const;

private:

    struct Impl;
    std::unique_ptr<Impl> impl;

    /// impl-getter for const correctness
    Impl* pimpl() noexcept;
    /// impl-getter for const correctness
    Impl const* pimpl() const noexcept;

    NodeFactory();
};

} // namespace intelli

#endif // GT_INTELLI_NODEFACTORY_H
