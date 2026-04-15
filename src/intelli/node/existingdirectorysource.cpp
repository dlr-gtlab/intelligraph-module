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

#include <gt_abstractproperty.h>

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
    connect(&m_value, &GtAbstractProperty::changed,
            this, [this]() { emit directoryChanged(m_value.get()); });

    emit directoryChanged(m_value.get());
}

QString
ExistingDirectorySourceNode::directory() const
{
    return m_value.get();
}

void
ExistingDirectorySourceNode::setDirectory(QString const& path)
{
    if (m_value.get() == path) return;
    m_value.setVal(path);
}

void
ExistingDirectorySourceNode::eval()
{
    setNodeData(m_out, std::make_shared<StringData>(m_value));
}
