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
    
    setNodeFlag(ResizableHOnly, true);

    m_in = addInPort(typeId<ObjectData>());
    m_out = addOutPort(makePort(typeId<ObjectData>()).setCaption(tr("child")));

    connect(&m_targetClassName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
    connect(&m_targetObjectName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    connect(&m_targetClassName, &GtAbstractProperty::changed,
            this, [this]() { emit targetClassNameChanged(m_targetClassName.get()); });
    connect(&m_targetObjectName, &GtAbstractProperty::changed,
            this, [this]() { emit targetObjectNameChanged(m_targetObjectName.get()); });
}

QString
FindDirectChildNode::targetClassName() const
{
    return m_targetClassName.get();
}

void
FindDirectChildNode::setTargetClassName(QString const& name)
{
    if (m_targetClassName.get() == name) return;
    m_targetClassName.setVal(name);
}

QString
FindDirectChildNode::targetObjectName() const
{
    return m_targetObjectName.get();
}

void
FindDirectChildNode::setTargetObjectName(QString const& name)
{
    if (m_targetObjectName.get() == name) return;
    m_targetObjectName.setVal(name);
}

ObjectData const*
FindDirectChildNode::inputObject() const
{
    return nodeData<ObjectData>(m_in).get();
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
        auto const& targetName = m_targetObjectName.get();

        if (targetClass.isEmpty() && targetName.isEmpty()) return false;

        bool classMatch = targetClass == c->metaObject()->className();
        bool nameMatch = targetName == c->objectName();

        // both specified -> search by class and object name
        if (!targetClass.isEmpty() && !targetName.isEmpty())
        {
            return (classMatch && nameMatch);
        }
        // only class name given -> search by class name
        if (!targetClass.isEmpty())
        {
            return classMatch;
        }
        // only object name given -> search by object name
        return nameMatch;
    });

    if (iter == std::end(children))
    {
        setNodeData(m_out, nullptr);
        return evalFailed();
    }

    setNodeData(m_out, std::make_shared<ObjectData>(*iter));
}
