/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igconditionalnode.h"

#include "gt_igbooldata.h"
#include "gt_igdoubledata.h"
#include "gt_intelligraphdatafactory.h"
#include "gt_intelligraphnodefactory.h"
#include "gt_igstringselectionpropertyitem.h"

#ifdef GTIG_DEVELOPER_PREVIEW
GTIG_REGISTER_NODE(GtIgConditionalNode, "Conditional")
#endif

GtIgConditionalNode::GtIgConditionalNode() :
    GtIntelliGraphNode("Conditional Node"),
    m_dataType("dataType", tr("Port Data Type"),
               GtIntelliGraphDataFactory::instance().knownClasses())
{
    registerProperty(m_dataType);

    m_inCondition = addInPort(PortData{
        gt::ig::typeId<GtIgBoolData>(), QStringLiteral("condition")
    }, Required);
    m_inData      = addInPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("data")
    });

    m_outIf   = addOutPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("if-branch")
    }, DoNotEvaluate);
    m_outElse = addOutPort(PortData{
        m_dataType.selectedValue(), QStringLiteral("else-branch")
    }, DoNotEvaluate);

    registerWidgetFactory([this](GtIntelliGraphNode&){
        auto w = std::make_unique<GtIgComboBox>();
        w->addItems(m_dataType.values());
        w->setCurrentText(m_dataType.selectedValue());

        auto const updateProp = [this, w_ = w.get()](){
            m_dataType.select(w_->currentIndex());
        };

        auto const update = [this, w_ = w.get()](){
            w_->setCurrentText(m_dataType.selectedValue());
        };

        connect(w.get(), &GtIgComboBox::focusOut, this, updateProp);
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

    connect(this, &GtIntelliGraphNode::inputDataRecieved, this, [=](PortIndex idx){
        if (auto* condition = nodeData<GtIgBoolData*>(m_inCondition))
        {
            auto* ifPort = port(m_outIf);
            auto* elsePort = port(m_outElse);
            if (!ifPort || !elsePort) return;

            ifPort->evaluate = condition->value();
            elsePort->evaluate = !condition->value();
        }
    }, Qt::DirectConnection);

    updatePorts();
}

GtIntelliGraphNode::NodeData
GtIgConditionalNode::eval(PortId outId)
{
    if (outId != m_outIf && outId != m_outElse) return {};

    gtInfo() << "EVALUATING (CONDITIONAL NODE)" << (nodeData<GtIgDoubleData*>(m_inData) ? nodeData<GtIgDoubleData*>(m_inData)->value() : 0.0);

    return nodeData(m_inData);
}

