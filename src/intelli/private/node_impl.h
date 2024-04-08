/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_NODE_IMPL_H
#define GT_INTELLI_NODE_IMPL_H

#include <intelli/memory.h>
#include <intelli/node.h>
#include <intelli/property/uint.h>

#include <gt_intproperty.h>
#include <gt_doubleproperty.h>
#include <gt_boolproperty.h>
#include <gt_exceptions.h>

namespace intelli
{

class NodeDataInterface;

template <typename Ports>
auto findPort(Ports&& ports, PortId id)
{
    return std::find_if(ports.begin(), ports.end(), [id](auto const& p){
        return p.id() == id;
    });
}

struct NodeImpl
{
    using PortInfo      = Node::PortInfo;
    using WidgetFactory = Node::WidgetFactory;

    NodeImpl(QString const& name) : modelName(name) { }

    /// node id
    UIntProperty id{
        "id", QObject::tr("Node Id"), QObject::tr("Node Id"), invalid<NodeId>()
    };

    /// x position of node
    GtDoubleProperty posX{
        "posX", QObject::tr("x-Pos"), QObject::tr("x-Position")
    };
    /// y position of node
    GtDoubleProperty posY{
        "posY", QObject::tr("y-Pos"), QObject::tr("y-Position")
    };

    /// width of node widget
    GtIntProperty sizeWidth{
        "sizeWidth",
        QObject::tr("Size width"),
        QObject::tr("Size width"), -1
    };
    /// height of node widget
    GtIntProperty sizeHeight{
         "sizeHeight",
         QObject::tr("Size height"),
         QObject::tr("Size height"), -1
    };
    /// whether node is active
    GtBoolProperty isActive{
         "isActive",
         QObject::tr("Is Node active"),
         QObject::tr("Is automatic Node evaluation active"),
         true
    };

    /// caption string
    QString modelName;
    /// ports
    std::vector<PortInfo> inPorts, outPorts{};
    /// factory for creating the widget
    WidgetFactory widgetFactory{};

    NodeDataInterface* dataInterface{};
    /// node flags
    NodeFlags flags{NodeFlag::DefaultNodeFlags};
    ///
    NodeEvalMode evalMode{NodeEvalMode::Default};
    /// iterator for the next port id
    PortId nextPortId{0};

    std::vector<PortInfo>& ports(PortType type) noexcept(false)
    {
        switch (type)
        {
        case PortType::In:
            return inPorts;
        case PortType::Out:
            return outPorts;
        case PortType::NoType:
            break;
        }

        throw GTlabException{
            __FUNCTION__, QStringLiteral("Invalid port type specified!")
        };
    }

    std::vector<PortInfo> const& ports(PortType type) const noexcept(false)
    {
        return const_cast<NodeImpl*>(this)->ports(type);
    }

    struct FindData
    {
        PortType type{PortType::NoType};
        PortIndex idx{};
        std::vector<PortInfo>* ports{};

        operator bool() const {
            return ports && type != PortType::NoType && idx != invalid<PortIndex>();
        }
    };

    FindData find(PortId id)
    {
        for (auto type : { PortType::In, PortType::Out })
        {
            auto& ports = this->ports(type);
            auto iter = findPort(ports, id);

            if (iter != ports.end())
            {
                auto idx = std::distance(ports.begin(), iter);
                return {type, PortIndex::fromValue(idx), &ports};
            }
        }
        return {};
    }

    PortId incNextPortId(PortId id)
    {
        if (id != invalid<PortId>())
        {
            // port already exists
            if (find(id)) return PortId{};

            if (id >= nextPortId) nextPortId = PortId(id + 1);

            return id;
        }
        return nextPortId++;
    }
};

} // namespace intelli

#endif // GT_INTELLI_NODE_IMPL_H
