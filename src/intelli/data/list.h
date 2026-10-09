/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LIST_H
#define GT_INTELLI_LIST_H

#include "intelli/graphconnectionmodel.h"
#include <intelli/data/invalid.h>
#include <intelli/nodedata.h>
#include <intelli/view.h>

#include <gt_typetraits.h>
#include <gt_utilities.h>

#include <QVector>

/// Helper macro to register a list type for a scalar type
#define GT_INTELLI_DECLARE_LIST_TYPE(SCALAR, LIST) \
    template <> \
    struct intelli::list_type<SCALAR> { using type = LIST; }; \
    template <> \
    struct intelli::inner_type<LIST> { using type = SCALAR; };

namespace intelli
{

/**
 * @brief Interface class for list types, enables to access individual indicies
 * of a list type.
 */
class GT_INTELLI_EXPORT ListData : public NodeData
{
    Q_OBJECT

public:

    struct NodeDataProxy
    {
        using Iter = gt::DynamicRange<size_t>::iterator;

        using value_type = NodeDataPtr;
        using reference  = value_type;
        using pointer    = value_type;

        ListData const* list{};

        /// initializes the proxy
        void init(Iter&) {}

        /// returns the underlying value type of iterator
        reference get(Iter& i) { return list->getAt(*i); }

        /// advances the underlying iterator
        void advance(Iter& i) { ++i; }
    };

    auto iterate() const
    {
        return makeProxy(
            gt::range(size_t{0}, getLength()),
            NodeDataProxy{this});
    }

    virtual bool append(NodeDataPtr const&) = 0;

protected:

    ListData(QString typeName) : NodeData(std::move(typeName)) {}

    virtual size_t getLength() const = 0;

    virtual NodeDataPtr getAt(size_t idx) const = 0;
};

/**
 * @brief Generic list data class able to hold any node data ptr
 */
class GT_INTELLI_EXPORT GenericListData final : public ListData
{
    Q_OBJECT

    using container_type = QVector<NodeDataPtr>;

public:

    using value_type      = gt::trait::value_t<container_type>;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;
    using size_type       = typename container_type::size_type;

    Q_INVOKABLE GenericListData();
    GenericListData(View<NodeDataPtr> const& list) : GenericListData()
    {
        std::for_each(list.begin(), list.end(), [this](NodeDataPtr const& d) {
            return this->append(d);
        });
    }

    bool append(NodeDataPtr const&  data) override;

    size_type size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    const_reference front() const { return m_data.front(); }
    const_reference back() const { return m_data.back(); }

    const_reference at(size_type idx) const { return m_data.at(idx); }

protected:

    size_t getLength() const override { return size(); }

    NodeDataPtr getAt(size_t idx) const override { return at(idx); }

private:

    QVector<NodeDataPtr> m_data;
};

template <typename T>
struct list_type
{
    using type = GenericListData;

    static_assert(!is_list_type<T>::value, "T is already a list type!");
    static_assert(std::is_base_of<NodeData, T>::value, "T must be derived of `intelli::NodeData`");
    static_assert(!std::is_same<T, InvalidData>::value, "Cannot use `intelli::InvalidData` as list type!");
};

} // namespace intelli

#endif // GT_INTELLI_LIST_H
