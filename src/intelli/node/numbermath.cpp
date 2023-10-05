/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 21.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node/numbermath.h"

#include "intelli/nodefactory.h"
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
    m_out = addOutPort(PortData{
        intelli::typeId<DoubleData>(),
        QStringLiteral("result") // custom port caption
    });

    registerWidgetFactory([=](){
        auto base = makeWidget();
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

intelli::Node::NodeDataPtr
NumberMathNode::eval(PortId outId)
{
    if (m_out != outId) return {};

    auto* dataA = nodeData<intelli::DoubleData*>(m_inA);
    auto* dataB = nodeData<intelli::DoubleData*>(m_inB);

    double a = 0.0, b = 0.0;

    if (dataA) a = dataA->value();
    if (dataB) b = dataB->value();

    switch (m_operation)
    {
    case MathOperation::Minus:
        return std::make_shared<intelli::DoubleData>(a - b);
    case MathOperation::Divide:
        if (b == 0.0)
        {
            gtWarning().verbose().nospace()
                << __FUNCTION__ << ": " << tr("Cannot divide by 0!");
            return {};
        }
        return std::make_shared<DoubleData>(a / b);
    case MathOperation::Multiply:
        return std::make_shared<DoubleData>(a * b);
    case MathOperation::Power:
        return std::make_shared<DoubleData>(std::pow(a, b));
    case MathOperation::Plus:
        break;
    }

    return std::make_shared<DoubleData>(a + b);
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
