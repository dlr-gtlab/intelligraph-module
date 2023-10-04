/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEDATA_H
#define GT_INTELLI_NODEDATA_H

#include <intelli/exports.h>

#include <gt_object.h>

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
class TemplateData : public NodeData
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
    TemplateData(QString typeName, T data = {}) :
        NodeData(std::move(typeName)),
        m_data(std::move(data))
    { }

private:

    T m_data;
};

} // namespace intelli

using GtIgNodeData [[deprecated]] = intelli::NodeData;

template <typename T>
using GtIgTemplateData [[deprecated]] = intelli::TemplateData<T>;

#endif // GT_INTELLI_NODEDATA_H
