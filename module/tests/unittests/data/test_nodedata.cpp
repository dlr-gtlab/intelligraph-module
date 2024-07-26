/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 16.2.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "test_nodedata.h"

#include <intelli/nodedatafactory.h>

void
TestNodeData::registerOnce()
{
    static auto _ = []{
          return GT_INTELLI_REGISTER_DATA(TestNodeData);
      }();
    Q_UNUSED(_);
}

TestNodeData::TestNodeData(double value) :
    intelli::NodeData(QStringLiteral("test")),
    m_value(value)
{

}

double
TestNodeData::myDouble() const
{
    return m_value;
}

double
TestNodeData::myDoubleModified(int i, QString const& s) const
{
    return m_value * (double)i * (double)s.size();
}

void
TestNodeData::setMyDouble(double value)
{
    m_value = value;
}

