/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  E-Mail: jens.schmeink@dlr.de
 */


#include "intelli/data/integer.h"

using namespace intelli;

IntegerData::IntegerData(int val) :
    NodeData("integer"),
    m_data(std::move(val))
{

}

int
IntegerData::value() const
{
    return m_data;
}
