/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/node/conditional.h"

#include "intelli/data/bool.h"
#include "intelli/data/double.h"
#include "intelli/nodedatafactory.h"
#include "intelli/nodefactory.h"
#include "intelli/gui/property_item/stringselection.h"

using namespace intelli;

#ifdef GTIG_DEVELOPER_PREVIEW
GT_INTELLI_REGISTER_NODE(ConditionalNode, "Conditional")
#endif

ConditionalNode::ConditionalNode() :
    Node("Conditional Node"),
    m_dataType("dataType", tr("Port Data Type"),
               NodeDataFactory::instance().knownClasses())
{
    registerProperty(m_dataType);

    m_inCondition = addInPort(PortData{
        typeId<BoolData>(), QStringLiteral("condition")
    }, Required);
    m_inData      = addInPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("data")
    });

    m_outIf = addOutPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("if-branch")
    }/*, DoNotEvaluate*/);
    m_outElse = addOutPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("else-branch")
    }/*, DoNotEvaluate*/);

    registerWidgetFactory([this](){
        auto w = std::make_unique<ComboBox>();
        w->addItems(m_dataType.values());
        w->setCurrentText(m_dataType.selectedValue());

        auto const updateProp = [this, w_ = w.get()](){
            m_dataType.select(w_->currentIndex());
        };

        auto const update = [this, w_ = w.get()](){
            w_->setCurrentText(m_dataType.selectedValue());
        };
        
        connect(w.get(), &ComboBox::focusOut, this, updateProp);
        connect(&m_dataType, &GtAbstractProperty::changed, w.get(), update);

        update();

        return w;
    });

    auto const updatePort = [this](PortId id){
        if (auto* port = this->port(id))
        {
            if (port->typeId != m_dataType.selectedValue())
            {
                port->typeId = m_dataType.selectedValue();
                emit portChanged(id);
            }
        }
    };
    auto const updatePorts = [=](){
//        gtDebug() << "Updating ports";
        updatePort(m_inData);
        updatePort(m_outIf);
        updatePort(m_outElse);
        updateNode();
    };

    connect(&m_dataType, &GtAbstractProperty::changed, this, updatePorts);
    
    connect(this, &Node::inputDataRecieved, this, [=](PortIndex idx){
        if (auto* condition = nodeData<BoolData*>(m_inCondition))
        {
            auto* ifPort = port(m_outIf);
            auto* elsePort = port(m_outElse);
            if (!ifPort || !elsePort) return;

//            ifPort->evaluate = condition->value();
//            elsePort->evaluate = !condition->value();
        }
    }, Qt::DirectConnection);

    updatePorts();
}

Node::NodeDataPtr
ConditionalNode::eval(PortId outId)
{
    if (outId != m_outIf && outId != m_outElse) return {};

    gtInfo() << "EVALUATING (CONDITIONAL NODE)" << (nodeData<DoubleData*>(m_inData) ? nodeData<DoubleData*>(m_inData)->value() : 0.0);

    return nodeData(m_inData);
}

