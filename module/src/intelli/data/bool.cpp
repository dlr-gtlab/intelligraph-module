/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/bool.h"

using namespace intelli;

BoolData::BoolData(bool val) :
    NodeData("bool"),
    m_data(std::move(val))
{

}

bool
BoolData::value() const
{
    return m_data;
}

