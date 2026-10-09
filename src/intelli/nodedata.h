/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEDATA_H
#define GT_INTELLI_NODEDATA_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <gt_logging.h>

#include <tl/optional.hpp>

#include <QMetaMethod>

#include <type_traits>

namespace intelli
{

class InvalidData;
class ListData;

/**
 * @brief The NodeData class. Base class for all node data
 */
class GT_INTELLI_EXPORT NodeData : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Type name. May be displayed in the editor as a default port caption
     * @return Type name
     */
    QString const& typeName() const;

    /**
     * @brief Type id of the node data. Is guranteed to be unique.
     * @return
     */
    [[deprecated("use `typeId<T>` instead")]]
    QString typeId() const;

    /**
     * @brief value
     * @param methodName - Name of an invokable method to call
     * @return pair of success and value of the call of an invokable function
     * of the node data object
     */

    /**
     * @brief Attempts to invoke the (Q_INVOKABLE) member method `methodName`.
     * @param methodName Name of an invokable method to call
     * @param args Additional optional arguments (use `Q_ARG(type, value), ...`)
     * @return `tl::optional` of T
     */
    template<typename T, typename... Args>
    std::enable_if_t<!std::is_void<T>::value, tl::optional<T>>
    invoke(QString const& methodName, Args&&... args) const
    {
        T var;

        int rtypeId = qMetaTypeId<T>();

        QByteArray const& rtypeName = QMetaType(rtypeId).name();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto&& retArg = Q_RETURN_ARG(T, var);
#else
        // Use normalized metatype name on Qt 5
        auto retArg = QReturnArgument<T>(rtypeName, var);
#endif

        auto* helper = const_cast<NodeData*>(this);

        if (!QMetaObject::invokeMethod(helper,
                                       methodName.toLatin1(),
                                       Qt::DirectConnection,
                                       retArg,
                                       std::forward<Args>(args)...))
        {
            gtTraceId("IntelliGraph")
                << tr("Invoking member function '%1 %2(...)' failed!")
                        .arg(rtypeName, methodName);
            return {};
        }

        return {var};
    }

protected:

    /**
     * @brief constructor
     * @param typeName Type name
     */
    NodeData(QString typeName);

private:

    QString m_typeName;
};


template<typename T>
using Ptr = std::shared_ptr<const T>;
template<typename T>
using NonConstPtr = std::shared_ptr<T>;

using NodeDataPtr = Ptr<NodeData>;
using NodeDataNonConstPtr = NonConstPtr<NodeData>;

/// returns the corresponding list type for T
template <typename T>
struct list_type;
template <typename T>
using list_type_t = typename list_type<T>::type;
template <typename T>
using list = list_type<T>;

/// returns the inner type T of a a list type
template <typename T>
struct inner_type { using type = void; };
template <typename T>
struct inner_type<list_type<T>> { using type = T; };
template <typename T>
using inner_type_t = typename inner_type<T>::type;

/// unwraps list_type
template <typename T>
struct unwrap_type { using type = T; };
template <typename T>
struct unwrap_type<list_type<T>> { using type = list_type_t<T>; };
template <typename T>
using unwrap_type_t = typename unwrap_type<T>::type;

/// returns whether a type T is a list type
template<typename T>
struct is_list_type
    : std::bool_constant<
          // either derived of `ListData`
          std::is_base_of<ListData, T>::value ||
          // or a wrapper type whose inner type is derived of `NodeData`
          (
              std::negation<std::is_same<inner_type_t<T>, void>>::value &&
              std::is_base_of<NodeData, inner_type_t<T>>::value
          )
      > {};

/**
 * @brief Returns the typeid of a node data class
 * @return Type id
 */
template <typename T,
          typename = std::enable_if_t<!is_list_type<T>::value>,
          typename = std::enable_if_t<std::is_base_of<NodeData, T>::value>>
inline QString typeId()
{
    return T::staticMetaObject.className();
}

/**
 * @brief Returns the list-typeid of a node data class
 * @return List type id
 */
template <typename T>
inline QString listTypeId()
{
    static_assert(!is_list_type<T>::value,
                  "`T` must not be a list type!");
    static_assert(!std::is_same<T, InvalidData>::value,
                  "Cannot use `intelli::InvalidData` as list type!");

    return QStringLiteral("#list#") + typeId<T>();
}

/**
 * @brief Overload. Returns the list-typeid of a list node data class
 * @return List type id
 */
template <typename T,
          typename = std::enable_if_t<is_list_type<T>::value>>
inline QString typeId()
{
    using U = inner_type_t<T>;
    static_assert(!std::is_same<U, void>::value,
                  "Could not interfer inner type of `T`!");
    static_assert(!std::is_same<U, InvalidData>::value,
                  "Cannot use `intelli::InvalidData` as list type!");
    static_assert(std::is_base_of<NodeData, U>::value,
                  "T::type must be derived of `intelli::NodeData`!");

    return listTypeId<U>();
}

/**
 * @brief Constructs NodeData of type T:
 *
 * makeNodeData<IntData>(...);
 *
 * @param args Arguments to initialize node data
 * @return Node data
 */
template <typename T,
          typename ...Args,
          typename = std::enable_if_t<std::is_base_of<NodeData, T>::value ||
                                      std::is_base_of<NodeData, inner_type_t<T>>::value>>
inline Ptr<unwrap_type_t<T>> makeNodeData(Args&&... args)
{
    return std::make_shared<unwrap_type_t<T> const>(std::forward<Args>(args)...);
}

/**
 * @brief Wrapper around `typeId<T>()` that returns the associated type-id if
 * it exists or an empty string if no compatible function call exists.
 * @return type-id (may be empty)
 */
template <typename T>
inline QString safeTypeId()
{
    using U = inner_type_t<T>;
    if constexpr ((is_list_type<T>::value && ( std::is_same<U, InvalidData>::value   ||
                                              !std::is_base_of<NodeData, U>::value)) ||
                  !std::is_base_of<NodeData, T>::value)
    {
        return QString{};
    }
    else
    {
        return typeId<T>();
    }
}

/**
 * @brief The TemplateData class. Helper class to allow for simple extension
 */

template <typename T>
class [[deprecated]] TemplateData : public NodeData
{
public:

    /**
     * @brief getter
     * @return value
     */
    T const& value() const { return m_data; }

protected:

    /**
     * @brief Type name. May be displayed in the editor as a default port caption
     * @param name Type name
     * @param data Data
     */
    [[deprecated("This template class is no longer supported and will be removed"
                 "in a future release. Use 'NodeData' instead and implement the"
                 "'value' function on your own.")]]
    TemplateData(QString typeName, T data = {}) :
        NodeData(std::move(typeName)),
        m_data(std::move(data))
    { }

private:

    T m_data;
};

} // namespace intelli

#endif // GT_INTELLI_NODEDATA_H
