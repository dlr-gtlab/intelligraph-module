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

#include <intelli/nodedata.h>
#include <intelli/data/invalid.h>

#include <gt_typetraits.h>

#include <QVector>

namespace intelli
{

class GT_INTELLI_EXPORT ListData : public NodeData
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

    Q_INVOKABLE ListData();

    size_type size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    const_reference front() const { return m_data.front(); }
    const_reference back() const { return m_data.back(); }

    const_reference at(size_type idx) const { return m_data.at(idx); }

private:

    container_type m_data;
};

template <typename T>
struct list_type
{
    using type = ListData;

    static_assert(!is_list_type<T>::value, "T is already a list type!");
    static_assert(std::is_base_of<NodeData, T>::value, "T must be derived of `intelli::NodeData`");
    static_assert(!std::is_same<T, InvalidData>::value, "Cannot use `intelli::InvalidData` as list type!");
};

template <>
inline QString typeId<ListData>()
{
    static_assert(is_list_type<ListData>::value, "Cannot use list type standalone!");
    return {};
}


} // namespace intelli

#endif // GT_INTELLI_LIST_H
