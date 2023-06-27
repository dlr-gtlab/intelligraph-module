/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGVOLATILEPTR_H
#define GT_IGVOLATILEPTR_H

#include <QPointer>

#include "gt_typetraits.h"

namespace gt
{
namespace ig
{

/// if you just dont know whether Qt took the ownership of your object...
/// (works like unique_ptr but checks if ptr exists using a QPointer under the hood)
template <typename T>
class volatile_ptr
{
public:

    explicit volatile_ptr(T* ptr = nullptr) :
        m_ptr(ptr)
    { }

    ~volatile_ptr()
    {
        if (m_ptr)
        {
            delete m_ptr;
        }
    }

    volatile_ptr(volatile_ptr const& o) = delete;
    volatile_ptr& operator=(volatile_ptr const& o) = delete;

    template <typename U = T, gt::trait::enable_if_base_of<T, U> = true>
    volatile_ptr(volatile_ptr<U>&& o) :
        m_ptr(o.release())
    { }

    template <typename U = T, gt::trait::enable_if_base_of<T, U> = true>
    volatile_ptr& operator=(volatile_ptr<U>&& o)
    {
        volatile_ptr tmp(std::move(o));
        swap(tmp);
        return *this;
    }

    void reset(T* ptr = nullptr)
    {
        if (m_ptr) delete m_ptr;
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

template <typename T, typename ...Args>
inline volatile_ptr<T> make_volatile(Args&&... args) noexcept
{
    return volatile_ptr<T>(new T{std::forward<Args>(args)...});
}

} // namespace ig

} // namespace gt

template <typename T>
inline void swap(gt::ig::volatile_ptr<T>& a, gt::ig::volatile_ptr<T>& b) noexcept
{
    a.swap(b);
}

#endif // GT_IGVOLATILEPTR_H
