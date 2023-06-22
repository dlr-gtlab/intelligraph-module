/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 5.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igfinddirectchildnode.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_igobjectdata.h"

#include <QRegExpValidator>

GTIG_REGISTER_NODE(GtIgFindDirectChildNode, "Object")

GtIgFindDirectChildNode::GtIgFindDirectChildNode() :
    GtIntelliGraphNode(tr("Find Direct Child")),
    m_childClassName("targetClassName",
                     tr("Target class name"),
                     tr("Target class name for child"))
{
    registerProperty(m_childClassName);

    m_inPort = addInPort(gt::ig::typeId<GtIgObjectData>(), PortPolicy::Required);

    addOutPort(gt::ig::typeId<GtIgObjectData>());

    registerWidgetFactory([this](GtIntelliGraphNode&) {
        auto w = std::make_unique<GtLineEdit>();
        w->setValidator(new QRegExpValidator(gt::re::ig::forClassNames()));
        w->setPlaceholderText(QStringLiteral("class name"));

        auto const updateProp = [this, w_ = w.get()](){
            m_childClassName = w_->text();
        };

        connect(w.get(), &GtLineEdit::focusOut, this, updateProp);
        connect(w.get(), &GtLineEdit::clearFocusOut, this, updateProp);
        connect(this, &GtIntelliGraphNode::outDataUpdated, w.get(),
                [this, w_ = w.get()](){
            w_->setText(m_childClassName);
        });
        return w;
    });

    connect(&m_childClassName, &GtAbstractProperty::changed,
            this, &GtIgFindDirectChildNode::updateNode);
}

GtIntelliGraphNode::NodeData
GtIgFindDirectChildNode::eval(PortId outId)
{
    if (auto* parent = portData<GtIgObjectData*>(m_inPort))
    {
        auto const children = parent->object()->findDirectChildren();

        auto iter = std::find_if(std::begin(children), std::end(children),
                                 [this](GtObject const* c){
            return m_childClassName.get() == c->metaObject()->className();
        });

        if (iter == std::end(children)) return {};

        return std::make_shared<GtIgObjectData>(*iter);
    }

    return {};
}
