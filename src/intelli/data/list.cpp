/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/list.h"

using namespace intelli;

GenericListData::GenericListData() :
    ListData("list")
{ }

bool
GenericListData::append(NodeDataPtr const& data)
{
    if (!data) return false;
    m_data.push_back(std::move(data));
    return true;
}

