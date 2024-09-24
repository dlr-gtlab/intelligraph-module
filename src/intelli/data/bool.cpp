/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/bool.h"

using namespace intelli;

BoolData::BoolData(bool val) :
    NodeData(QStringLiteral("bool")),
    m_data(val)
{

}

bool
BoolData::value() const
{
    return m_data;
}

