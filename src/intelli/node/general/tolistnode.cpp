
#include "intelli/node/general/tolistnode.h"
#include "gt_qtutilities.h"
#include "intelli/data/double.h"
#include "intelli/data/list.h"
#include "intelli/nodedatafactory.h"

using namespace intelli;

ToListNode::ToListNode() :
    DynamicNode("To List", DynamicInput | NoDefaultListTypes),
    m_selectedTypeId("typeId",
                     tr("Type Id"),
                     NodeDataFactory::instance().validTypeIds(),
                     intelli::typeId<DoubleData>())
{
    auto updatePort = [this](PortType type, PortIndex idx){
        TypeId const typeId = (type != PortType::Out) ?
            m_selectedTypeId.get() :
            NodeDataFactory::listType(m_selectedTypeId.get());

        PortId const portId = this->portId(type, idx);
        NodePort* port = this->port(portId);

        bool update = false;
        if (port->typeId != typeId)
        {
            port->typeId = typeId;
            update = true;
        }

        if (type == PortType::In && port->caption != QString::number(idx + 1))
        {
            port->caption = QString::number(idx + 1);
            update = true;
        }

        if (update) emit portChanged(portId);
    };

    auto updatePorts = [this, updatePort](){
        if (NodeDataFactory::isListType(m_selectedTypeId.get())) return;

        PortIndex idx{0};
        for (NodePort const& _ : ports(PortType::In))
        {
            Q_UNUSED(_);
            updatePort(PortType::In, idx++);
        }

        idx = PortIndex{0};
        for (NodePort const& _ : ports(PortType::Out))
        {
            Q_UNUSED(_);
            updatePort(PortType::Out, idx++);
        }
    };

    connect(&m_selectedTypeId, &StringSelectionProperty::changed, this, updatePorts);
    connect(this, &DynamicNode::portInserted, this, updatePort);

    m_out = addStaticOutPort(makePort(listTypeId<DoubleData>()).setCaption("out"));
    addStaticInPort(makePort(typeId<DoubleData>()));

    registerProperty(m_selectedTypeId);
}

void
ToListNode::eval()
{
    NonConstPtr<BaseListData> outputData = gt::unique_qobject_cast<BaseListData>(
        NodeDataFactory::instance().makeListData(m_selectedTypeId)
    );
    if (!outputData) return evalFailed();

    for (NodePort const& port : ports(PortType::In))
    {
        outputData->append(nodeData(port.id()));
    }

    setNodeData(m_out, outputData);
}

Node::PortId
ToListNode::insertPort(PortOption option, PortType type, PortInfo port, int idx)
{
    port.typeId = m_selectedTypeId;
    return DynamicNode::insertPort(option, type, std::move(port), idx);
}

