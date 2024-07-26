/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/nodedata.h"

using namespace intelli;

QString const&
NodeData::typeName() const
{
    return m_typeName;
}

QString
NodeData::typeId() const
{
    return metaObject()->className();
}

NodeData::NodeData(QString typeName) :
    m_typeName(std::move(typeName))
{

}
