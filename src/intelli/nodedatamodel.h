/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 4.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef NODEDATAINTERFACE_H
#define NODEDATAINTERFACE_H

#include <intelli/exports.h>
#include <intelli/globals.h>

namespace intelli
{

class Node;
class Graph;

namespace data_model
{

struct PortDataItem
{
    /// referenced port
    PortId portId;
    /// actual data at port
    NodeDataSet data{nullptr};
};

struct DataItem
{
    static constexpr size_t PRE_ALLOC = 8;

    explicit DataItem() {}

    /// in and out ports
    QVarLengthArray<PortDataItem, PRE_ALLOC> portsIn{}, portsOut{};
    /// internal evalution state
    NodeEvalState state = NodeEvalState::Outdated;

    bool isPending = false;

    /**
     * @brief Returns the ancestors or descendants depending on the port type
     * @param type Port type
     * @return Port vector
     */
    auto& ports(PortType type)
    {
        assert(type != PortType::NoType);
        return (type == PortType::In) ? portsIn : portsOut;
    }
    auto const& ports(PortType type) const
    {
        return const_cast<DataItem*>(this)->ports(type);
    }

    /**
     * @brief Returns the port data item associated with `portId`. If `typeOut`
     * is not null, it will hold whether the port is an output or input.
     * @param portId Port id to find
     * @param typeOut Out: Port type
     * @return Port data item
     */
    PortDataItem* findPort(PortId portId, PortType* typeOut = nullptr)
    {
        PortType type = PortType::In;
        for (auto* ports : {&portsIn, &portsOut})
        {
            auto portIter = std::find_if(ports->begin(), ports->end(),
                                         [portId](PortDataItem const& port){
                                             return port.portId == portId;
            });

            if (portIter != ports->end())
            {
                if (typeOut) *typeOut = type;
                return portIter;
            }
            type = PortType::Out;
        }
        return {};
    }
    PortDataItem const* findPort(PortId portId, PortType* typeOut = nullptr) const
    {
        return const_cast<DataItem*>(this)->findPort(portId, typeOut);
    }
};

using GraphDataModel = QHash<NodeUuid, DataItem>;

} // namespace data_model

using data_model::GraphDataModel;

/**
 * @brief The NodeDataInterface class.
 * Interface to access and set the data of a node port
 */
class NodeDataInterface
{
public:

    virtual ~NodeDataInterface() = default;

    virtual NodeDataSet nodeData(NodeUuid const& nodeUuid, PortId portId) const = 0;
    virtual NodeDataPtrList nodeData(NodeUuid const& nodeUuid, PortType type) const = 0;

    virtual bool setNodeData(NodeUuid const& nodeUuid, PortId portId, NodeDataSet data) = 0;
    virtual bool setNodeData(NodeUuid const& nodeUuid, PortType type, NodeDataPtrList const& data) = 0;
};

} // namespace intelli

#endif // NODEDATAINTERFACE_H
