/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/textdisplay.h>

#include <intelli/data/string.h>
#include <intelli/data/bytearray.h>

using namespace intelli;

TextDisplayNode::TextDisplayNode() :
    Node("Text Display"),
    m_textType("textType", tr("Text Type"), tr("Text Type"), TextType::PlainText)
{
    registerProperty(m_textType);

    setNodeEvalMode(NodeEvalMode::Blocking);
    setNodeFlag(Resizable, true);

    addInPort(makePort(typeId<StringData>())
                  .setCaptionVisible(false));
}

QString
TextDisplayNode::displayText() const
{
    auto const& inPorts = ports(PortType::In);
    if (inPorts.empty()) return {};

    auto const& data = nodeData<StringData>(inPorts.front().id());
    return data ? data->value() : QString{};
}

TextDisplayNode::TextType
TextDisplayNode::textType() const
{
    return m_textType.getVal();
}
