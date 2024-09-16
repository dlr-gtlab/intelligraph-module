/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_VOLATILEPTR_H
#define GT_INTELLI_VOLATILEPTR_H

#include <gt_typetraits.h>

#include <QPointer>

namespace intelli
{

/// Helper struct for `volatile_ptr` to schedule the deletion of an object
struct DeferredDeleter
{
    template <typename T>
    void operator()(T* ptr) const { ptr->deleteLater(); }
};

/// Helper struct for `volatile_ptr` to directly delete an object
struct DirectDeleter
{
    template <typename T>
    void operator()(T* ptr) const { delete ptr; }
};


/// if you just dont know whether Qt took the ownership of your object...
/// (works like unique_ptr but checks if ptr exists using a QPointer under the hood)
template <typename T, typename Deleter = DeferredDeleter>
class volatile_ptr
{
public:

    volatile_ptr(std::nullptr_t) :
        volatile_ptr((T*)nullptr)
    { }
    explicit volatile_ptr(T* ptr = nullptr) :
        m_ptr(ptr)
    { }

    ~volatile_ptr()
    {
        if (m_ptr) reset();
    }

    volatile_ptr(volatile_ptr const& o) = delete;
    volatile_ptr& operator=(volatile_ptr const& o) = delete;

    template <typename U = T, gt::trait::enable_if_base_of<T, U> = true>
    volatile_ptr(volatile_ptr<U, Deleter>&& o) :
        m_ptr(o.release())
    { }

    template <typename U = T, gt::trait::enable_if_base_of<T, U> = true>
    volatile_ptr& operator=(volatile_ptr<U, Deleter>&& o)
    {
        volatile_ptr tmp(std::move(o));
        swap(tmp);
        return *this;
    }

    void reset(T* ptr = nullptr)
    {
        if (m_ptr) Deleter{}(m_ptr.data());
        m_ptr = ptr;
    }

    T* release()
    {
        auto* p = get();
        m_ptr.clear();
        return p;
    }

    T* get() { return m_ptr; }
    T const* get() const { return m_ptr; }

    operator T* () { return get(); }
    operator T const* () const { return get(); }

    T& operator *() { return *m_ptr; }
    T const& operator *() const { return *m_ptr; }

    T* operator ->() { return get(); }
    T const* operator ->() const { return get(); }

    template <typename U = T, gt::trait::enable_if_base_of<U, T> = true>
    operator volatile_ptr<U>&() { return *this; }

    template <typename U = T, gt::trait::enable_if_base_of<U, T> = true>
    operator volatile_ptr<U> const&() const { return *this; }

    void swap(volatile_ptr& o) noexcept
    {
        using std::swap; // ADL
        swap(m_ptr, o.m_ptr);
    }

private:

    QPointer<T> m_ptr;
};

template <typename T, typename Deleter = DeferredDeleter, typename ...Args>
inline volatile_ptr<T, Deleter> make_volatile(Args&&... args) noexcept
{
    return volatile_ptr<T, Deleter>(new T{std::forward<Args>(args)...});
}

} // namespace intelli

template <typename T>
inline void swap(intelli::volatile_ptr<T>& a,
                 intelli::volatile_ptr<T>& b) noexcept
{
    a.swap(b);
}

#endif // GT_INTELLI_VOLATILEPTR_H
