/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/finddirectchild.h"

#include "intelli/data/object.h"

#include <gt_lineedit.h>

#include <QRegExpValidator>

using namespace intelli;

FindDirectChildNode::FindDirectChildNode() :
    Node(tr("Find Direct Child")),
    m_childClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child"))
{
    registerProperty(m_childClassName);
    
    m_in = addInPort(typeId<ObjectData>(), PortPolicy::Required);
    
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

Node::NodeDataPtr
FindDirectChildNode::eval(PortId outId)
{
    if (m_out != outId) return {};
    
    if (auto* parent = nodeData<ObjectData*>(m_in))
    {
        auto const children = parent->object()->findDirectChildren();

        auto iter = std::find_if(std::begin(children), std::end(children),
                                 [this](GtObject const* c){
            return m_childClassName.get() == c->metaObject()->className();
        });

        if (iter == std::end(children)) return {};
        
        return std::make_shared<ObjectData>(*iter);
    }

    return {};
}
