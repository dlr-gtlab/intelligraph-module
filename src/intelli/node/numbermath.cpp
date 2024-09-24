/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/numbermath.h"

#include "intelli/data/double.h"

#include <QComboBox>
#include <QLayout>

using namespace intelli;

NumberMathNode::NumberMathNode() :
    Node("Math Node"),
    m_operation("operation", tr("Math Operation"), tr("Math Operation"), MathOperation::Plus)
{
    registerProperty(m_operation);

    // in ports
    m_inA = addInPort(typeId<DoubleData>());
    m_inB = addInPort(typeId<DoubleData>());

    // out ports
    m_out = addOutPort(PortInfo{
        typeId<DoubleData>(),
        QStringLiteral("result") // custom port caption
    });

    registerWidgetFactory([=](){
        auto base = makeBaseWidget();
        auto w = new QComboBox();
        base->layout()->addWidget(w);
        w->addItems(QStringList{"+", "-", "*", "/", "pow"});

        auto const update = [this, w](){
            w->setCurrentText(toString(m_operation));
        };

        connect(&m_operation, &GtAbstractProperty::changed, w, update);

        connect(w, &QComboBox::currentTextChanged,
                this, [this, w](){
            auto tmp = toMathOperation(w->currentText());
            if (tmp == m_operation) return;

            m_operation = tmp;
            updatePortCaptions();
        });

        update();

        return base;
    });

    updatePortCaptions();

    connect(&m_operation, &GtAbstractProperty::changed,
            this, &Node::triggerNodeEvaluation);
}

void
NumberMathNode::eval()
{
    auto dataA = nodeData<DoubleData>(m_inA);
    auto dataB = nodeData<DoubleData>(m_inB);
    if (!dataA && !dataB)
    {
        setNodeData(m_out, nullptr);
        return;
    }

    double a = 0.0, b = 0.0;

    if (dataA) a = dataA->value();
    if (dataB) b = dataB->value();

    double c = 0.0;

    switch (m_operation)
    {
    case MathOperation::Minus:
        c = a - b;
        break;
    case MathOperation::Divide:
        if (b == 0.0)
        {
            gtWarning().verbose().nospace()
                << __FUNCTION__ << ": " << tr("Cannot divide by 0!");
            return;
        }
        c = a / b;
        break;
    case MathOperation::Multiply:
        c = a * b;
        break;
    case MathOperation::Power:
        c = std::pow(a, b);
        break;
    case MathOperation::Plus:
        c = a + b;
        break;
    }

    setNodeData(m_out, std::make_shared<DoubleData>(c));
}

QString
NumberMathNode::toString(MathOperation op) const
{
    switch (op)
    {
    case MathOperation::Minus:
        return QStringLiteral("-");
    case MathOperation::Divide:
        return QStringLiteral("/");
    case MathOperation::Multiply:
        return QStringLiteral("*");
    case MathOperation::Power:
        return QStringLiteral("pow");
    case MathOperation::Plus:
        break;
    }
    return QStringLiteral("+");
}

NumberMathNode::MathOperation
NumberMathNode::toMathOperation(QString const& op) const
{
    if (op == QStringLiteral("-")) return MathOperation::Minus;
    if (op == QStringLiteral("*")) return MathOperation::Multiply;
    if (op == QStringLiteral("/")) return MathOperation::Divide;
    if (op == QStringLiteral("pow")) return MathOperation::Power;
    return MathOperation::Plus;
}

void
NumberMathNode::updatePortCaptions()
{
    auto* inA = port(m_inA);
    auto* inB = port(m_inB);
    assert(inA); assert(inB);

    switch (m_operation)
    {
    case MathOperation::Minus:
        inA->caption = QStringLiteral("minuend");
        inB->caption = QStringLiteral("subtrahend");
        break;
    case MathOperation::Divide:
        inA->caption = QStringLiteral("dividend");
        inB->caption = QStringLiteral("divisor");
        break;
    case MathOperation::Multiply:
        inA->caption = QStringLiteral("multiplier");
        inB->caption = QStringLiteral("multiplicand");
        break;
    case MathOperation::Power:
        inA->caption = QStringLiteral("base");
        inB->caption = QStringLiteral("exponent");
        break;
    case MathOperation::Plus:
        inA->caption = QStringLiteral("summand A");
        inB->caption = QStringLiteral("summand B");
        break;
    }

    emit nodeChanged();
}
