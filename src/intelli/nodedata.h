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

#include <gt_logging.h>
#include <gt_object.h>
#include <thirdparty/tl/optional.hpp>

#include <QMetaMethod>

namespace intelli
{

/**
 * @brief The NodeData class. Base class for all node data
 */
class GT_INTELLI_EXPORT NodeData : public GtObject
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
     * @param val0 Additional optional argument 0 (use `Q_ARG(type, value)`)
     * @param val1 Additional optional argument 1 (use `Q_ARG(type, value)`)
     * @param val2 Additional optional argument 2 (use `Q_ARG(type, value)`)
     * @param val3 Additional optional argument 3 (use `Q_ARG(type, value)`)
     * @param val4 Additional optional argument 4 (use `Q_ARG(type, value)`)
     * @param val5 Additional optional argument 5 (use `Q_ARG(type, value)`)
     * @param val6 Additional optional argument 6 (use `Q_ARG(type, value)`)
     * @param val7 Additional optional argument 7 (use `Q_ARG(type, value)`)
     * @param val8 Additional optional argument 8 (use `Q_ARG(type, value)`)
     * @param val9 Additional optional argument 9 (use `Q_ARG(type, value)`)
     * @return `tl::optional` of T
     */
    template <typename T>
    tl::optional<T> invoke(QString const& methodName,
                           QGenericArgument val0 = {},
                           QGenericArgument val1 = {},
                           QGenericArgument val2 = {},
                           QGenericArgument val3 = {},
                           QGenericArgument val4 = {},
                           QGenericArgument val5 = {},
                           QGenericArgument val6 = {},
                           QGenericArgument val7 = {},
                           QGenericArgument val8 = {},
                           QGenericArgument val9 = {}) const
    {
        T var;

        int rtypeId = qMetaTypeId<T>();

        QByteArray const& rtypeName = QMetaType(rtypeId).name();

        // Qt is expecting a "normalized" type name here (not "typeid(T).name()")
        auto retarg = QReturnArgument<T>(rtypeName, var);

        auto* helper = const_cast<NodeData*>(this);

        if (!QMetaObject::invokeMethod(helper,
                                       methodName.toLatin1(),
                                       Qt::DirectConnection,
                                       retarg, val0, val1, val2, val3,
                                       val4, val5, val6, val7, val8, val9))
        {
            gtTraceId("IntelliGraph")
                << tr("Invoking meber function '%1 %2(...)' failed!")
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

/**
 * @brief Returns the typeid of a node data class
 * @return Typeid
 */
template <typename T, gt::trait::enable_if_base_of<NodeData, T> = true>
inline QString typeId()
{
    return T::staticMetaObject.className();
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
