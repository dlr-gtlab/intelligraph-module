/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 19.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/logicoperation.h"

#include <intelli/data/bool.h>

#include <QComboBox>
#include <QLayout>

using namespace intelli;

LogicNode::LogicNode() :
    Node("Logic Operation"),
    m_operation("operation", tr("Logic Operation"), tr("Logic Operation"), LogicOperation::AND)
{
    registerProperty(m_operation);

    // in ports
    m_inA = addInPort(typeId<BoolData>());
    m_inB = addInPort(typeId<BoolData>());

    // out ports
    m_out = addOutPort(typeId<BoolData>());

    /*
    registerWidgetFactory([=](){
        auto base = makeBaseWidget();
        auto w = new QComboBox();
        base->layout()->addWidget(w);
        w->addItems(QStringList{"NOT", "AND", "OR", "XOR", "NAND", "NOR"});

        auto const update = [this, w](){
            w->setCurrentText(toString(m_operation));
        };

        connect(&m_operation, &GtAbstractProperty::changed, w, update);

        connect(w, &QComboBox::currentTextChanged,
                this, [this, w](){
            auto tmp = toLogicOperation(w->currentText());
            if (tmp == m_operation) return;

            m_operation = tmp;
        });

        update();

        return base;
    });
    */

    connect(&m_operation, &GtAbstractProperty::changed, this, [this](){
        if (m_operation == LogicOperation::NOT && port(m_inB))
        {
            removePort(m_inB);
        }
        else if (!port(m_inB))
        {
            addInPort(PortInfo::customId(m_inB, typeId<BoolData>()));
        }
        emit nodeChanged();
        emit triggerNodeEvaluation();
    });
}

LogicNode::LogicOperation
LogicNode::operation() const
{
    return m_operation;
}

void
LogicNode::eval()
{
    bool a = false, b = false, c = false;

    if (auto tmp = nodeData<BoolData>(m_inA)) a = tmp->value();

    if (m_operation != NOT)
    {
        if (auto tmp = nodeData<BoolData>(m_inB)) b = tmp->value();
    }

    switch (m_operation)
    {
    case NOT:
        c = !a; break;
    case AND:
        c = a & b; break;
    case OR:
        c = a | b; break;
    case XOR:
        c = a ^ b; break;
    case NAND:
        c = !(a & b); break;
    case NOR:
        c = !(a | b); break;
    }

    setNodeData(m_out, std::make_shared<BoolData>(c));
}

QString
LogicNode::toString(LogicOperation op) const
{
    switch (op)
    {
    case LogicOperation::AND:
        return QStringLiteral("AND");
    case LogicOperation::OR:
        return QStringLiteral("OR");
    case LogicOperation::XOR:
        return QStringLiteral("XOR");
    case LogicOperation::NAND:
        return QStringLiteral("NAND");
    case LogicOperation::NOR:
        return QStringLiteral("NOR");
    case LogicOperation::NOT:
        break;
    }
    return QStringLiteral("NOT");
}

LogicNode::LogicOperation
LogicNode::toLogicOperation(const QString& op) const
{
    if (op == QStringLiteral("AND")) return LogicOperation::AND;
    if (op == QStringLiteral("OR")) return LogicOperation::OR;
    if (op == QStringLiteral("XOR")) return LogicOperation::XOR;
    if (op == QStringLiteral("NAND")) return LogicOperation::NAND;
    if (op == QStringLiteral("NOR")) return LogicOperation::NOR;
    return LogicOperation::NOT;
}

