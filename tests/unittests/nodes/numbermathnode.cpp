/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 10.8.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "numbermathnode.h"

#include "intelli/nodefactory.h"
#include "intelli/data/double.h"

using namespace intelli;
using namespace intelli::test;

#include <QComboBox>

GT_INTELLI_REGISTER_NODE(NumberMathNode, "Test");

NumberMathNode::NumberMathNode() :
    Node("Math Node"),
    m_operation("operation", tr("Math Operation"), tr("Math Operation"), MathOperation::Plus)
{
    registerProperty(m_operation);

    // in ports
    m_inA = addInPort({
        typeId<intelli::DoubleData>(),
        QStringLiteral("input A")
    }, Optional);
    m_inB = addInPort({
        typeId<intelli::DoubleData>(),
        QStringLiteral("input B")
    }, Optional);

    // out ports
    m_out = addOutPort(PortData{
        intelli::typeId<intelli::DoubleData>(),
        QStringLiteral("result") // custom port caption
    });

    registerWidgetFactory([=](intelli::Node&){
        auto w = std::make_unique<QComboBox>();
        w->addItems(QStringList{"+", "-", "*", "/"});

        auto const update = [this, w_ = w.get()](){
            w_->setCurrentText(toString(m_operation));
        };

        connect(&m_operation, &GtAbstractProperty::changed, w.get(), update);

        connect(w.get(), &QComboBox::currentTextChanged,
                this, [this, w_ = w.get()](){
                    auto tmp = toMathOperation(w_->currentText());
                    if (tmp != m_operation) m_operation = tmp;
                });

        update();

        return w;
    });

    connect(&m_operation, &GtAbstractProperty::changed, this, &Node::updateNode);
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
            gtWarning() << tr("Cannot divide by 0!");
            return {};
        }
        return std::make_shared<intelli::DoubleData>(a / b);
    case MathOperation::Multiply:
        return std::make_shared<intelli::DoubleData>(a * b);
    case MathOperation::Plus:
    default:
        break;
    }

    return std::make_shared<intelli::DoubleData>(a + b);
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
    case MathOperation::Plus:
    default:
        return QStringLiteral("+");
    }
}

NumberMathNode::MathOperation
NumberMathNode::toMathOperation(QString const& op) const
{
    if (op == QStringLiteral("-")) return MathOperation::Minus;
    if (op == QStringLiteral("*")) return MathOperation::Multiply;
    if (op == QStringLiteral("/")) return MathOperation::Divide;
    return MathOperation::Plus;
}


