/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/stringbuilder.h>

#include <intelli/data/string.h>

using namespace intelli;

StringBuilderNode::StringBuilderNode() :
    Node("String Builder"),
    m_pattern("pattern", tr("Pattern"),
              tr("String Builder Pattern"),
              QStringLiteral("%1%2"))
{
    setNodeFlag(ResizableHOnly, true);

    registerProperty(m_pattern);

    m_inA = addInPort({typeId<StringData>(), tr("%1")});
    m_inB = addInPort({typeId<StringData>(), tr("%2")});
    m_out = addOutPort({typeId<StringData>(), tr("new_str")});

    connect(&m_pattern, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
    connect(&m_pattern, &GtAbstractProperty::changed,
            this, [this]() { emit patternChanged(m_pattern.get()); });
}

QString
StringBuilderNode::pattern() const
{
    return m_pattern.get();
}

void
StringBuilderNode::setPattern(QString const& pattern)
{
    if (m_pattern.get() == pattern) return;
    m_pattern.setVal(pattern);
}

void
StringBuilderNode::eval()
{
    QString a, b;

    if (auto dataA = nodeData<StringData>(m_inA)) a = dataA->value();
    if (auto dataB = nodeData<StringData>(m_inB)) b = dataB->value();

    QString result = m_pattern.get();

    if (result.contains("%1")) result = result.arg(a);
    if (result.contains("%2")) result = result.arg(b);

    setNodeData(m_out, std::make_shared<StringData>(std::move(result)));
}
