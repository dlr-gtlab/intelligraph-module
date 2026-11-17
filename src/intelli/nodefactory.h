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
#include <gt_platform.h>

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
     * @brief Returns list of registered nodes
     * @return list of nodes
     */
    GT_NO_DISCARD
    QStringList registeredNodes() const;

    /**
     * @brief Returns unsorted list of categories
     * @return Unsorted list of categories
     */
    GT_NO_DISCARD
    QStringList registeredCategories() const;

    /**
     * @brief Returns sorted list of categories according to their order
     * priorities or alphabetic ordering. Ascending.
     * @return Sorted list of categories
     */
    GT_NO_DISCARD
    QStringList sortedCategories() const;

    /**
     * @brief Returns all associated categories of the given node. View is
     * invalidated when registering a new node
     * @param className Node's class name
     * @return Categories.
     */
    GT_NO_DISCARD
    View<QString> nodeCategories(QString const& className) const;
    [[deprecated("use `nodeCategories` instead")]]
    GT_NO_DISCARD
    QString nodeCategory(QString const& className) const;

    /**
     * @brief Returns the model name (= display name) of the given node
     * @param className Node's class name
     * @return Model name
     */
    GT_NO_DISCARD
    QString nodeModelName(QString const& className) const;

    /**
     * @brief Returns true if class is a known (registered) node class
     * @param className Class name
     * @return Whether class is known
     */
    GT_NO_DISCARD
    bool isKnownNode(QString const& className) const;

    /**
     * @brief Registers the node so that it can be used by IntelliGraph
     * @param meta Metaobject of the node
     * @param category Category to list node in. Use an empty string to "hide"
     * the node in the scene menu.
     * @param moduleId Module Id of the associated node. Must not be empty.
     * @return Whether registration was successful
     */
    bool registerNode(QMetaObject const& meta, QString const& category, QString moduleId);

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
              typename = std::enable_if_t<std::is_base_of<Node, T>::value>,
              typename = std::enable_if_t<std::is_constructible<T>::value>>
    inline bool registerNode(QString const& category)
    {
        return registerNode(T::staticMetaObject, category, GT_MODULENAME());
    }

    /**
     * @brief Template based overload. Registers the Node `T`.
     * @tparam T Node
     * @param categories Categories to add node to
     * @return Whether registration was successful
     */
    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Node, T>::value>,
              typename = std::enable_if_t<std::is_constructible<T>::value>>
    inline bool registerNode(View<QString> categories)
    {
        return registerNode(T::staticMetaObject, categories, GT_MODULENAME());
    }

    /**
     * @brief Instantiates a new node of type className. May throw
     * `std::logic_error` if operation fails.
     * @param className Class to instantiate
     * @return Object pointer (never null)
     */
    GT_NO_DISCARD
    [[deprecated("use `tryMakeNode` instead, which returns null "
                 "instead of throwing if creation fails!")]]
    std::unique_ptr<Node> makeNode(QString const& className) const noexcept(false);

    /**
     * @brief Instantiates a new node of type className. Does not throw an
     * exception but has still no noexcept guarantee.
     * @param className Class to instantiate
     * @return Object pointer (may be null)
     */
    GT_NO_DISCARD
    std::unique_ptr<Node> tryMakeNode(QString const& className) const;

    /**
     * @brief Sets the order priority of a category. Succeeds even if category
     * was not registered yet. The order priority is used for sorting the
     * categories as returned by `sortedCategories`. Lower priorities are
     * placed before higher values.
     * @param category Category to set order priority of.
     * @param value Category order Priority
     */
    void setCategoryOrderPriority(QString const& category, int value);

    /**
     * @brief Returns the registered order priority for the given category.
     * Lower priorities are placed before higher values.
     * @param category Category
     * @return Returns the order priority. If the category is unknown
     * `CategoryOrderPriority::Normal` is returned.
     */
    int categoryOrderPriority(QString const& category) const;

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
    using GtAbstractObjectFactory::moduleId;
#endif

protected:
    /// constructor
    NodeFactory();

private:

    struct Impl;
    std::unique_ptr<Impl> impl;

    /// impl-getter for const correctness
    Impl* pimpl() noexcept;
    /// impl-getter for const correctness
    Impl const* pimpl() const noexcept;
};

} // namespace intelli

#endif // GT_INTELLI_NODEFACTORY_H
