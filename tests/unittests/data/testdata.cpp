/* 
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 * 
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "testdata.h"

#include <intelli/nodedatafactory.h>
#include <intelli/node.h>

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

TestListData::TestListData() :
    intelli::ListData(QStringLiteral("testlist"))
{ }

QString
TestListData::typeId() const
{
    return intelli::typeId<TestListData>();
}

bool
TestListData::append(intelli::NodeDataPtr const& data)
{
    if (!data) return false;

    auto converted = intelli::convert<TestNodeData>(data);
    if (!converted) return false;

    m_data.push_back(std::move(converted));
    return true;
}

size_t
TestListData::getSize() const noexcept
{
    return m_data.size();
}

intelli::NodeDataPtr
TestListData::getAt(size_t idx) const
{
    return m_data.at(idx);
}

