/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_ignodedata.h"

QString const&
GtIgNodeData::typeName() const
{
    return m_typeName;
}

QString
GtIgNodeData::typeId() const
{
    return metaObject()->className();
}

GtIgNodeData::GtIgNodeData(QString typeName) :
    m_typeName(std::move(typeName))
{

}
