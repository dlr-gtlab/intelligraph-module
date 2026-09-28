/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "intelli/data/stringlist.h"

using namespace intelli;

StringListData::StringListData(QStringList val) :
    BaseListData(QStringLiteral("stringlist")),
    m_data(std::move(val))
{

}

QStringList
intelli::StringListData::value() const
{
    return m_data;
}

void
intelli::StringListData::setValue(QStringList val)
{
    m_data = std::move(val);
}

bool
StringListData::append(NodeDataPtr const& data)
{
    Ptr<StringData> stringData = convert<StringData>(std::move(data));
    if (!stringData) return false;

    m_data.push_back(stringData->value());
    return true;
}
