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

StringSelectionNode::StringSelectionNode() :
    Node(tr("String Selection")),
    m_selection("selectedString", tr("selection"), tr("selection"))
{
    m_in = addInPort({typeId<StringListData>(), tr("list")});
    m_out = addOutPort(makePort(typeId<StringData>()).setCaption(tr("selection")));

    registerProperty(m_selection);
    m_selection.hide();

    setNodeFlag(ResizableHOnly, true);
    
    auto const updateOptions = [this]() {
        QStringList given;
        if (auto list = nodeData<StringListData>(m_in))
        {
            given = list->value();
        }
        emit optionsChanged(given);

        if (given.isEmpty())
        {
            setSelection(QString{});
            return;
        }

        if (!given.contains(m_selection))
        {
            setSelection(given.first());
        }
    };

    connect(this, &Node::inputDataRecieved,
            this, [this, updateOptions](PortId portId)
            {
                if (portId != m_in) return;
                updateOptions();
            });

    updateOptions();
}

QString
StringSelectionNode::selection() const
{
    return m_selection.get();
}

void
StringSelectionNode::setSelection(QString const& selection)
{
    if (m_selection.get() == selection) return;
    m_selection = selection;
    emit selectionChanged(m_selection.get());
    emit triggerNodeEvaluation();
}

QStringList
StringSelectionNode::options() const
{
    if (auto list = nodeData<StringListData>(m_in))
    {
        return list->value();
    }
    return {};
}

void
StringSelectionNode::eval()
{
    auto list = nodeData<StringListData>(m_in);
    if (!list)
    {
        setNodeData(m_out, nullptr);
        return evalFailed();
    }

    QStringList listValues = list->value();

    if (listValues.contains(m_selection))
    {
        setNodeData(m_out, std::make_shared<StringData>(m_selection));
        return;
    }

    setNodeData(m_out, std::make_shared<StringData>(listValues.first()));
}
