/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "intelli/data/string.h"

using namespace intelli;

StringData::StringData(QString val):
    NodeData(QStringLiteral("string")),
    m_data(std::move(val))
{

}

QString
StringData::value() const
{
    return m_data;
}
