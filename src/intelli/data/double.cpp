/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/double.h"

using namespace intelli;

DoubleData::DoubleData(double val) :
    NodeData(QStringLiteral("double")),
    m_data(val)
{

}

double
DoubleData::value() const
{
    return m_data;
}
