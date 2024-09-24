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

#include <gt_lineedit.h>

#include <QRegExpValidator>
#include <QRegExp>

namespace gt
{
namespace re
{

namespace intelli
{

inline QRegExp forClassNames()
{
    return QRegExp(R"(^([a-zA-Z_][a-zA-Z0-9_]*::)*[a-zA-Z_][a-zA-Z0-9_]*$)");
}

} // namespace intelli

} // namespace re

} // namespace gt

using namespace intelli;

FindDirectChildNode::FindDirectChildNode() :
    Node(tr("Find Direct Child")),
    m_childClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child"))
{
    registerProperty(m_childClassName);
    
    m_in = addInPort(typeId<ObjectData>());
    
    m_out = addOutPort(typeId<ObjectData>());

    registerWidgetFactory([this]() {
        auto w = std::make_unique<GtLineEdit>();
        w->setValidator(new QRegExpValidator(gt::re::intelli::forClassNames()));
        w->setPlaceholderText(QStringLiteral("class name"));

        auto const updateProp = [this, w_ = w.get()](){
            m_childClassName = w_->text();
        };
        auto const updateText = [this, w_ = w.get()](){
            w_->setText(m_childClassName);
        };

        connect(w.get(), &GtLineEdit::focusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::clearFocusOut, this, updateProp);
        connect(&m_childClassName, &GtAbstractProperty::changed, w.get(), updateText);

        updateText();

        return w;
    });

    connect(&m_childClassName, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
FindDirectChildNode::eval()
{
    auto* parent = nodeData<ObjectData*>(m_in);
    if (!parent)
    {
        setNodeData(m_out, nullptr);
        return;
    }

    auto const children = parent->object()->findDirectChildren();

    auto iter = std::find_if(std::begin(children), std::end(children),
                             [this](GtObject const* c){
        return m_childClassName.get() == c->metaObject()->className();
    });

    if (iter != std::end(children))
    {
        setNodeData(m_out, std::make_shared<ObjectData>(*iter));
    }
}
