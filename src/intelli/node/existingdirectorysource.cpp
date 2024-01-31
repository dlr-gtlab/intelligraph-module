/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 22.01.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
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
    setNodeFlag(intelli::Resizable);

    m_out = addOutPort(typeId<StringData>());

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
