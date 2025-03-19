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
    NodeData(QStringLiteral("stringlist")),
    m_data(std::move(val))
{

}

QString
intelli::StringListData::value() const
{

}

void
intelli::StringListData::setValue(const QStringList &val)
{

}
