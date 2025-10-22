/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/booldisplay.h>
#include <intelli/data/bool.h>

using namespace intelli;

BoolDisplayNode::BoolDisplayNode() :
    Node(QStringLiteral("Bool Display")),
    m_displayMode("displayMode",
                  tr("Display Mode"),
                  tr("Display Mode"))
{
    registerProperty(m_displayMode);

    setNodeEvalMode(NodeEvalMode::Blocking);

    m_in = addInPort(makePort(typeId<BoolData>())
                         .setCaptionVisible(false));
}
