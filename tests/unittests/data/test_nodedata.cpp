/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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

