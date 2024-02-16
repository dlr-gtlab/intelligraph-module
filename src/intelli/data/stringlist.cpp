/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/stringlist.h"

using namespace intelli;


StringListData::StringListData(QStringList list) :
    NodeData("stringlist"),
    m_data(std::move(list))
{

}

QStringList
StringListData::value() const
{
    return m_data;
}
