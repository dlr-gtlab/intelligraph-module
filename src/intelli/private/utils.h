/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_UTILS_H
#define GT_INTELLI_UTILS_H

#include <intelli/globals.h>
#include <intelli/data/double.h>

#include <gt_logstream.h>
#include <gt_platform.h>

#include <gt_intproperty.h>

#define GT_INTELLI_PROFILE() \
intelli::Profiler profiler__{__FUNCTION__}; (void)profiler__;
#define GT_INTELLI_PROFILE_C(X) \
intelli::Profiler profiler__{X}; (void)profiler__;

inline gt::log::Stream&
operator<<(gt::log::Stream& s, std::shared_ptr<intelli::NodeData const> const& data)
{
    // TODO: remove
    if (auto* d = qobject_cast<intelli::DoubleData const*>(data.get()))
    {
        gt::log::StreamStateSaver saver(s);
        return s.nospace() << data->metaObject()->className() << " (" <<d->value() << ")";
    }

    return s << (data ? data->metaObject()->className() : "nullptr");
}

namespace intelli
{

/// using std::is_const
template<typename T>
using is_const = std::is_const<std::remove_reference_t<T>>;

/// apply const to `T` if `IsConst` is true
template<bool IsConst, typename T>
struct apply_const;
template<typename T>
struct apply_const<true, T> { using type = std::add_const_t<T>; };
template<typename T>
struct apply_const<false, T> { using type = std::remove_const_t<T>; };

/// wrapper to apply const to `T` if `IsConst` is true
template<bool IsConst, typename T>
using const_t = typename apply_const<IsConst, T>::type;
/// wrapper to apply constness of `U` to `T`
template<typename U, typename T>
using apply_constness_t = typename apply_const<is_const<U>::value, T>::type;

/// Retrieves iterator type for `QHash` depending on `IsConst`
template<bool IsConst, typename T>
struct get_iterator;
template<typename T>
struct get_iterator<true, T> { using type = typename T::const_iterator; };
template<typename T>
struct get_iterator<false, T> { using type = typename T::iterator; };
template<bool IsConst, typename T>
using get_iterator_t = typename get_iterator<IsConst, T>::type;

/// Profiler helper class
class Profiler
{
public:
    Profiler(char const* text = "") :
        m_text(text),
        m_start(std::chrono::high_resolution_clock::now())
    { }
    ~Profiler()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

        if (duration > 0) gtTrace() << "[PROFILER]" << m_text << "- took" << duration << "us";
    }

private:
    char const* m_text = {};
    std::chrono::high_resolution_clock::time_point m_start;
};

/// Converts `t` into a `QString` using `gt::log::Stream`
template <typename T>
inline QString toString(T const& t)
{
    gt::log::Stream s;
    s.nospace() << t;
    return QString::fromStdString(s.str());
}

/// Returns the node path to the root graph node
template <typename N>
inline QString
relativeNodePath(N const& node)
{
    N const* root = node.template findRoot<std::remove_reference_t<N>*>();
    if (!root) return node.caption();

    return root->caption() + (node.objectPath().remove(root->objectPath())).replace(';', '/');
}

namespace utils
{

/// Helper function that searches for `t` in List and returns the iterator
template<typename List, typename T>
inline auto
find(List& list, T const& t)
{
    return std::find(list.begin(), list.end(), t);
}

/// Helper function that searches for `t` in List and returns the iterator
template<typename List, typename T>
inline auto
contains(List const& list, T const& t)
{
    return std::find(list.begin(), list.end(), t) != list.end();
}

/// Helper function that searches for `t` in List and erases it if `t` was found
template<typename List, typename T>
inline bool
erase(List& list, T const& t)
{
    auto iter = find(list, t);
    if (iter != list.end())
    {
        list.erase(iter);
        return true;
    }
    return false;
}

} // namespace utils

} // namespace intelli

#endif // GT_INTELLI_UTILS_H
