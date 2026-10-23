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

#include <QStringList>

namespace intelli
{

class GT_INTELLI_EXPORT StringListData final : public ListData
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

    Q_INVOKABLE StringListData(QStringList values = {});
    StringListData(View<QString> view);

    Q_INVOKABLE QStringList value() const;

    Q_INVOKABLE void setValue(QStringList val);

    /// Returns the associated list type id
    QString typeId() const override;

    /// Appends the data to the list. Data must be string compatible.
    /// Returns whether appending was successfull
    bool append(NodeDataPtr const& data) override;
    /// Appends the string to the list.
    void append(QString entry);

    /// Returns the size of this list
    size_type size() const noexcept { return m_data.size(); }
    /// Returns whether this list is empty
    bool empty() const noexcept { return m_data.empty(); }

    /// Returns begin iterator
    const_iterator begin() const noexcept { return m_data.begin(); }
    /// Returns end iterator
    const_iterator end() const noexcept { return m_data.end(); }

    /// Returns first element. List must not be empty
    const_reference front() const noexcept { return m_data.front(); }
    /// Returns last element. List must not be empty
    const_reference back() const noexcept { return m_data.back(); }

    /// Returns the element at the given position. Must not exceed list's size
    const_reference at(size_type idx) const noexcept { return m_data.at(idx); }

    /// Returns the element at the given position. Must not exceed list's size
    inline const_reference operator[](size_t idx) const noexcept { return at(idx); }

protected:

    /// Returns the size of the underlying list
    size_t getSize() const noexcept override
    {
        return size();
    }

    /// Returns the element at the given position
    NodeDataPtr getAt(size_t idx) const override
    {
        return makeNodeData<StringData>(at(idx));
    }

private:

    QStringList m_data;
};

} // namespace intelli

GT_INTELLI_DECLARE_LIST_TYPE(intelli::StringData, intelli::StringListData);

#endif // GT_INTELLI_STRINGLISTDATA_H
