/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/list.h"
#include "intelli/nodedatafactory.h"
#include "intelli/node.h"

using namespace intelli;

GenericListData::GenericListData(TypeId type) :
    ListData("list"),
    m_typeId(std::move(type))
{
    assert(!NodeDataFactory::instance().isListType(m_typeId));
}

bool
GenericListData::append(NodeDataPtr const& data)
{
    if (!data) return false;
    if (m_typeId.isEmpty())
    {
        TypeId newTypeId = data->metaObject()->className();
        if (!NodeDataFactory::instance().isKnownType(newTypeId)) return false;

        // should not be a list type, safe to use class name as type id
        m_typeId = newTypeId;
    }

    m_data.push_back(convert(data, m_typeId));
    return true;
}

bool
GenericListData::appendNoConvert(NodeDataPtr const& data)
{
    if (!data) return false;
    m_data.push_back(data);
    return true;
}

