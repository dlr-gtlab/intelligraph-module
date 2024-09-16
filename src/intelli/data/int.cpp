/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/int.h"

using namespace intelli;

IntData::IntData(int val) :
    NodeData(QStringLiteral("int")),
    m_data(val)
{

}

int
IntData::value() const
{
    return m_data;
}
