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

namespace gt
{

namespace ig
{

template <typename T>
inline QString typeId()
{
    return T::staticMetaObject.className();
}

} // namespace ig

} // namespace gt

class GT_IG_EXPORT GtIgNodeData : public GtObject
{
    Q_OBJECT

public:

    QString const& typeName() const;

    QString typeId() const;

protected:

    GtIgNodeData(QString typeName);

private:

    QString m_typeName;
};

#endif // GT_IGNODEDATA_H
