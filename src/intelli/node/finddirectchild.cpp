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
    m_childClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child")),
    m_objectName("TargetobjectName", tr("ObjectName"), tr("Object Name"))
{
    registerProperty(m_childClassName);
    registerProperty(m_objectName);
    
    setNodeFlag(Resizable, true);

    m_in = addInPort(typeId<ObjectData>());
    
    m_out = addOutPort(typeId<ObjectData>());

    registerWidgetFactory([this]() {
        auto w = std::make_unique<FindDirectChildNodeWidget>();

        w->updateNameCompleter(nodeData<intelli::ObjectData>(m_in).get());

        connect(w.get(), SIGNAL(updateClass(QString)),
                this, SLOT(updateClass(QString)));

        connect(&m_childClassName, SIGNAL(changed()),
                w.get(), SLOT(updateClassText()));

        connect(w.get(), SIGNAL(updateObjectName(QString)),
                this, SLOT(updateObjName(QString)));

        connect(&m_objectName, SIGNAL(changed()),
                w.get(), SLOT(updateNameText()));

        connect(this, SIGNAL(emitCompleterUpdate(const ObjectData*)),
                w.get(), SLOT(updateNameCompleter(const ObjectData*)));

        w->setClassNameWidget(m_childClassName.getVal());
        w->setObjectNameWidget(m_objectName.getVal());

        return w;
    });

    connect(this, SIGNAL(inputDataRecieved(PortId)),
            SLOT(onInputDataRecevied()));

    connect(&m_childClassName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
    connect(&m_objectName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
FindDirectChildNode::eval()
{
    auto parent = nodeData<ObjectData>(m_in);
    if (!parent)
    {
        gtTrace() << "FindDirectChildNode" << tr("Invalid parent");
        setNodeData(m_out, nullptr);
        return;
    }

    auto const children = parent->object()->findDirectChildren();

    auto iter = std::find_if(std::begin(children), std::end(children),
                             [this](GtObject const* c)
    {
        bool classCheck =
        m_childClassName.get() == c->metaObject()->className();

        bool nameCheck = true;
        if (!m_objectName.getVal().isEmpty())
        {
            nameCheck = m_objectName.get() == c->objectName();
        }

        return classCheck && nameCheck;
    });

    if (iter != std::end(children))
    {
        setNodeData(m_out, std::make_shared<intelli::ObjectData>(*iter));
    }
}

void
FindDirectChildNode::updateClass(const QString& newClass)
{
    m_childClassName.setVal(newClass);
}

void
FindDirectChildNode::updateObjName(const QString& newObjName)
{
    m_objectName.setVal(newObjName);
}

void
FindDirectChildNode::onInputDataRecevied()
{
    emit emitCompleterUpdate(nodeData<intelli::ObjectData>(m_in).get());
}
