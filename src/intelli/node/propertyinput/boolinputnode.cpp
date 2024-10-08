/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "boolinputnode.h"

#include <intelli/data/bool.h>

#include <QCheckBox>

#include <memory>

namespace intelli
{
BoolInputNode::BoolInputNode() :
    AbstractInputNode("Bool Input",
                      std::make_unique<GtBoolProperty>("value", tr("Value"),
                                                       tr("Current Value")))
{
    m_value->hide();

    m_out = addOutPort(intelli::typeId<intelli::BoolData>());
    port(m_out)->captionVisible = false;

    connect(m_value.get(), &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    registerWidgetFactory([this]() {
        auto w = std::make_unique<QCheckBox>();

        auto const updateProp = [this, w_ = w.get()](){
            if(value() != w_->isChecked()) setValue(w_->isChecked());
        };
        auto const updateText = [this, w_ = w.get()](){
            if (w_->isChecked() != value()) w_->setChecked(value());
        };

        connect(w.get(), &QCheckBox::stateChanged, this, updateProp);
        connect(m_value.get(), &GtAbstractProperty::changed,
                w.get(), updateText);

        updateText();

        return w;
    });
}

bool
BoolInputNode::value() const
{
    auto prop = static_cast<GtBoolProperty*>(m_value.get());
    return prop->getVal();
}

void
BoolInputNode::setValue(bool value)
{
    auto prop = static_cast<GtBoolProperty*>(m_value.get());
    prop->setVal(value);
}

void
BoolInputNode::eval()
{
    setNodeData(m_out, std::make_shared<intelli::BoolData>(value()));
}
} // namespace intelli
