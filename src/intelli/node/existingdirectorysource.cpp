/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include "intelli/node/existingdirectorysource.h"
#include "intelli/data/string.h"
#include "intelli/nodedata.h"

#include <gt_propertyfilechoosereditor.h>

using namespace intelli;

ExistingDirectorySourceNode::ExistingDirectorySourceNode():
    Node("Existing Directory"),
    m_value("directory", tr("Directory"), tr("Directory"))
{
    registerProperty(m_value);

    setFlag(GtObject::UserRenamable);
    setNodeFlag(Resizable);

    m_out = addOutPort(makePort(typeId<StringData>()).setCaptionVisible(false));

    connect(&m_value, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);

    registerWidgetFactory([=](){
        auto w = std::make_unique<GtPropertyFileChooserEditor>();
        w->setMinimumWidth(120);
        w->setFileChooserProperty(&m_value);

        return w;
    });
}

void
ExistingDirectorySourceNode::eval()
{
    setNodeData(m_out, std::make_shared<StringData>(m_value));
}
