/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/data/double.h"

using namespace intelli;

DoubleData::DoubleData(double val) :
    intelli::NodeData("double"),
    m_data(std::move(val))
{

}

double
DoubleData::value() const
{
    return m_data;
}