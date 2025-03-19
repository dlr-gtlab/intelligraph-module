/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#include "intelli/node/stringselection.h"

#include "intelli/data/string.h"
#include "intelli/data/stringlist.h"

using namespace intelli;

StringSelection::StringSelection() :
    Node(tr("String Selection"))
{
    m_in = addInPort(typeId<StringListData>());
    m_out = addOutPort(makePort(typeId<StringData>()).setCaption(tr("Selection")));
}

void
StringSelection::eval()
{
    auto list = nodeData<StringListData>(m_in);
    if (!list)
    {
        setNodeData(m_out, nullptr);
        return evalFailed();
    }

    QStringList listValues = list->value();


    setNodeData(m_out, std::make_shared<StringData>(listValues.first()));
}
