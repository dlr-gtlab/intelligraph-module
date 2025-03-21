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

#include <QComboBox>

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
    
    registerWidgetFactory([this]() {
        auto w = std::make_unique<QComboBox>();

        QStringList given;

        if (auto list = nodeData<StringListData>(m_in))
        {
            given = list->value();
        }

        w->addItems(given);

        m_selection = given.isEmpty() ? QString{} : given.first();

        connect(w.get(), &QComboBox::currentTextChanged,
                this, [this](QString const& newObjName) {
                    this->m_selection = newObjName;
                    emit triggerNodeEvaluation();
                });

        connect(this, &Node::inputDataRecieved,
                w.get(), [this, wid = w.get()]()
                {
                    wid->clear();
                    QStringList given;
                    auto list = nodeData<StringListData>(m_in);
                    if (list)
                    {
                        given = list->value();
                    }
                    wid->addItems(given);
                    emit triggerNodeEvaluation();
                });

        return w;
    });
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
