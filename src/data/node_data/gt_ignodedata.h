/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGNODEDATA_H
#define GT_IGNODEDATA_H

#include "gt_object.h"
#include "gt_intelligraph_exports.h"

/**
 * @brief The GtIgNodeData class. Base class for all node data
 */
class GT_IG_EXPORT GtIgNodeData : public GtObject
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
    GtIgNodeData(QString typeName);

private:

    QString m_typeName;
};

/**
 * @brief The GtIgTemplateData class. Helper class to allow for simple extension
 */
template <typename T>
class GtIgTemplateData : public GtIgNodeData
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
    GtIgTemplateData(QString typeName, T data = {}) :
        GtIgNodeData(std::move(typeName)),
        m_data(std::move(data))
    { }

private:

    T m_data;
};

namespace gt
{

namespace ig
{

/**
 * @brief Returns the typeid of a node data class
 * @return Typeid
 */
template <typename T, gt::trait::enable_if_base_of<GtIgNodeData, T> = true>
inline QString typeId()
{
    return T::staticMetaObject.className();
}

} // namespace ig

} // namespace gt

#endif // GT_IGNODEDATA_H
