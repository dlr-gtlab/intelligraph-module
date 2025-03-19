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
    Node(tr("String Selection"))
{
    m_in = addInPort(typeId<StringListData>());
    m_out = addOutPort(makePort(typeId<StringData>()).setCaption(tr("Selection")));

    registerWidgetFactory([this]() {
        auto w = std::make_unique<QComboBox>();

        QStringList given;
        auto list = nodeData<StringListData>(m_in);
        if (list)
        {
            given = list->value();
        }

        w->addItems(given);

        m_selection = given.first();

        connect(w.get(), &QComboBox::currentTextChanged,
                this, [this](QString const& newObjName) {
                    m_selection = newObjName;
                });

        connect(this, &Node::inputDataRecieved,
                w.get(), [this, wid = w.get(), &w]()
                {
                    w->clear();
                    QStringList given;
                    auto list = nodeData<StringListData>(m_in);
                    if (list)
                    {
                        given = list->value();
                    }

                    w->addItems(given);
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

    gtWarning() << tr("Invalid value selection");

}
