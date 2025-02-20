/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHDATAMODEL_H
#define GT_INTELLI_GRAPHDATAMODEL_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <gt_finally.h>

namespace intelli
{

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
    /// counter of nodes that are running in subgraph nodes
    size_t evaluatingChildNodes = 0;

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
     * @return Port data item
     */
    PortDataItem* findPort(PortId portId)
    {
        for (auto* ports : {&portsIn, &portsOut})
        {
            auto portIter = std::find_if(ports->begin(), ports->end(),
                                         [portId](PortDataItem const& port){
                return port.portId == portId;
            });

            if (portIter != ports->end())
            {
                return portIter;
            }
        }
        return {};
    }

    PortDataItem const* findPort(PortId portId) const
    {
        return const_cast<DataItem*>(this)->findPort(portId);
    }
};

using GraphDataModel = QHash<NodeUuid, DataItem>;

} // namespace data_model

using data_model::GraphDataModel;

} // namespace intelli

#endif // GT_INTELLI_GRAPHDATAMODEL_H
