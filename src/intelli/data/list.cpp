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

ListData::ListData() :
    BaseListData("list")
{

}

bool
ListData::append(NodeDataPtr const& data)
{
    m_data.push_back(std::move(data));
    return true;
}

