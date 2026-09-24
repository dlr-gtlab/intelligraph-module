/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#ifndef GT_INTELLI_STRINGLISTDATA_H
#define GT_INTELLI_STRINGLISTDATA_H

#include <intelli/nodedata.h>
#include <intelli/data/string.h>
#include <intelli/data/list.h>

namespace intelli
{

class GT_INTELLI_EXPORT StringListData : public BaseListData
{
    Q_OBJECT

    using container_type = QStringList;

public:

    using value_type      = gt::trait::value_t<container_type>;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;
    using size_type       = typename container_type::size_type;

    Q_INVOKABLE StringListData(QStringList val = {});

    Q_INVOKABLE QStringList value() const;

    [[deprecated("use the constructor or `append` instead")]]
    Q_INVOKABLE void setValue(QStringList val);

    bool append(NodeDataPtr const& data) override;

    size_type size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }

    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }

    const_reference front() const { return m_data.front(); }
    const_reference back() const { return m_data.back(); }

    const_reference at(size_type idx) const { return m_data.at(idx); }

protected:

    size_t getLength() const override { return size(); }

    NodeDataPtr getAt(size_t idx) const override { return makeNodeData<StringData>(at(idx)); }

private:

    QStringList m_data;
};

template <>
struct list_type<StringData> { using type = StringListData; };
template <>
struct inner_type<StringListData> { using type = StringData; };

} // namespace intelli

#endif // GT_INTELLI_STRINGLISTDATA_H
