/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/finddirectchild.h"

#include "intelli/data/object.h"

#include "intelli/gui/property_item/finddirectchildnodewidget.h"

#include <gt_lineedit.h>

#include <QRegExpValidator>
#include <QRegExp>

using namespace intelli;

FindDirectChildNode::FindDirectChildNode() :
    Node(tr("Find Direct Child")),
    m_targetClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child")),
    m_targetObjectName("targetObjectName",
                         tr("Target object name"),
                         tr("Target object name"))
{
    registerProperty(m_targetClassName);
    registerProperty(m_targetObjectName);
    
    setNodeFlag(Resizable, true);

    m_in = addInPort(typeId<ObjectData>());
    
    m_out = addOutPort(typeId<ObjectData>());

    registerWidgetFactory([this]() {
        auto w = std::make_unique<FindDirectChildNodeWidget>();

        w->updateNameCompleter(nodeData<ObjectData>(m_in).get());

        connect(w.get(), &FindDirectChildNodeWidget::updateClass,
                this, [this](QString const& newClass) {
                    m_targetClassName.setVal(newClass);
                });
        connect(&m_targetClassName, SIGNAL(changed()),
                w.get(), SLOT(updateClassText()));

        connect(w.get(), &FindDirectChildNodeWidget::updateObjectName,
                this, [this](QString const& newObjName) {
                    m_targetObjectName.setVal(newObjName);
                });
        connect(&m_targetObjectName, SIGNAL(changed()),
                w.get(), SLOT(updateNameText()));

        connect(this, &Node::inputDataRecieved,
                w.get(), [this, wid = w.get()](){
            wid->updateNameCompleter(nodeData<ObjectData>(m_in).get());
        });

        w->setClassNameWidget(m_targetClassName.getVal());
        w->setObjectNameWidget(m_targetObjectName.getVal());

        return w;
    });

    connect(&m_targetClassName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
    connect(&m_targetObjectName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
FindDirectChildNode::eval()
{
    auto parent = nodeData<ObjectData>(m_in);
    if (!parent)
    {
        setNodeData(m_out, nullptr);
        return evalFailed();
    }

    auto const children = parent->object()->findDirectChildren();

    auto iter = std::find_if(std::begin(children), std::end(children),
                             [this](GtObject const* c) {
        auto const& targetClass = m_targetClassName.get();
        bool classMatch = targetClass == c->metaObject()->className();
        if (!classMatch) return false;

        auto const& targetName = m_targetObjectName.get();
        bool nameMatch = targetName.isEmpty() || targetName == c->objectName();
        return nameMatch;
    });

    if (iter == std::end(children))
    {
        setNodeData(m_out, nullptr);
        return evalFailed();
    }

    setNodeData(m_out, std::make_shared<ObjectData>(*iter));
}
