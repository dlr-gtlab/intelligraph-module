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

#include <intelli/proxyiterator.h>
#include <intelli/nodedata.h>
#include <intelli/view.h>

#include <gt_utilities.h>

#include <QVector>

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

    /// Should return the associated list type id
    QString typeId() const override = 0;

    /// translates idx -> NodeDataPtr
    struct NodeDataProxy
    {
        using Iter = gt::DynamicRange<size_t>::iterator;

        using value_type = NodeDataPtr;
        using reference  = value_type;
        using pointer    = value_type;

        ListData const* list{};

        /// Initializes the proxy
        void init(Iter&) {}

        /// Returns the underlying value type of iterator
        reference get(Iter& i) { return list->getAt(*i); }

        /// Advances the underlying iterator
        void advance(Iter& i) { ++i; }
    };

    /// Returns an iterable
    auto iterate() const
    {
        return makeProxy(gt::range(size_t{0}, size()), NodeDataProxy{this});
    }

    /// Returns the size of this list
    size_t size() const noexcept { return getSize(); }
    /// Returns whether this list is empty
    bool empty() const noexcept { return size() == 0; }

    /// Returns begin iterator
    auto begin() const noexcept { return iterate().begin(); }
    /// Returns end iterator
    auto end() const noexcept { return iterate().end(); }

    /// Returns first element. List must not be empty
    NodeDataPtr front() const noexcept { return at(0); }
    /// Returns last element. List must not be empty
    NodeDataPtr back() const noexcept { return at(size() - 1); }

    /// Returns the element at the given position. Must not exceed list's size
    NodeDataPtr at(size_t idx) const
    {
        assert(size() > idx);
        return getAt(idx);
    }

    /// Returns the element at the given position. Must not exceed list's size
    inline NodeDataPtr operator[](size_t idx) const { return at(idx); }

    /// Appends the data to the list. Returns whether appending was successful
    virtual bool append(NodeDataPtr const&) = 0;

protected:

    /// constructor
    ListData(QString typeName) : NodeData(std::move(typeName)) {}

    /// Should return the size of the underlying list
    virtual size_t getSize() const noexcept = 0;

    /// Should return the element at the given position
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

    using value_type      = typename container_type::value_type;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;
    using size_type       = typename container_type::size_type;

    /// Constructor. The given type id is the type this list may accept. Must
    /// be a scalar type!
    Q_INVOKABLE GenericListData(TypeId id = {});

    template <typename List,
              typename Iterator = decltype(std::declval<List const&>().begin()),
              typename DataPtr = typename std::iterator_traits<Iterator>::value_type,
              typename T = unwrap_ptr_t<DataPtr>,
              typename = std::enable_if_t<std::is_same<GenericListData, list_type_t<T>>::value>>
    GenericListData(List const& list) :
        GenericListData(intelli::typeId<T>())
    {
        std::for_each(list.begin(), list.end(), [this](NodeDataPtr const& d) {
            return this->appendNoConvert(d);
        });
    }
    template <typename DataPtr,
              typename T = unwrap_ptr_t<DataPtr>,
              typename = std::enable_if_t<std::is_same<GenericListData, list_type_t<T>>::value>>
    GenericListData(std::initializer_list<DataPtr> const& list) :
        GenericListData(intelli::typeId<T>())
    {
        std::for_each(list.begin(), list.end(), [this](NodeDataPtr const& d) {
            return this->appendNoConvert(d);
        });
    }

    /// Returns the associated list type id
    QString typeId() const override;

    /// Reserves space for at least `size` elements
    void reserve(size_type size) { m_data.reserve(size); }

    /// Appends the data to the list. Returns whether appending was successful
    bool append(NodeDataPtr const& data) override;

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
    inline const_reference operator[](size_type idx) const { return at(idx); }

protected:

    /// Returns the size of the underlying list
    size_t getSize() const noexcept override { return size(); }

    /// Returns the element at the given position
    NodeDataPtr getAt(size_t idx) const override { return at(idx); }

private:

    QVector<NodeDataPtr> m_data;
    TypeId m_typeId;

    /// appends without the extra step of converting
    bool appendNoConvert(NodeDataPtr const& data);
};

// by default assumes GenericListData data type
template <typename T>
struct list_type
{
    // conditionally uses GenericListData
    using type = std::conditional_t<is_scalar_type<T>::value, GenericListData, void>;
};

} // namespace intelli

#endif // GT_INTELLI_LIST_H
