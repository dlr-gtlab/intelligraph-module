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
#include <intelli/exec/executor.h>

#include <gt_intproperty.h>
#include <gt_doubleproperty.h>
#include <gt_exceptions.h>

namespace intelli
{

template <typename Ports>
auto findPort(Ports&& ports, PortId id)
{
    return std::find_if(ports.begin(), ports.end(), [id](auto const& p){
        return p.id() == id;
    });
}

struct NodeImpl
{
    using NodeDataPtr   = Node::NodeDataPtr;
    using PortData      = Node::PortData;
    using WidgetFactory = Node::WidgetFactory;

    NodeImpl(QString const& name) : modelName(name) {}

    /// node id
    GtIntProperty id{"id",
                     QObject::tr("Node Id"),
                     QObject::tr("Node Id")};

    /// x position of node
    GtDoubleProperty posX{"posX",
                          QObject::tr("x-Pos"),
                          QObject::tr("x-Position")};
    /// y position of node
    GtDoubleProperty posY{"posY",
                          QObject::tr("y-Pos"),
                          QObject::tr("y-Position")};

    /// width of node widget
    GtIntProperty sizeWidth{"sizeWidth",
                            QObject::tr("Size width"),
                            QObject::tr("Size width"),
                            GtUnit::None, -1};
    /// height of node widget
    GtIntProperty sizeHeight{"sizeHeight",
                             QObject::tr("Size height"),
                             QObject::tr("Size height"),
                             GtUnit::None, -1};

    /// caption string
    QString modelName;
    /// ports
    std::vector<PortData> inPorts, outPorts{};
    /// data
    std::vector<NodeDataPtr> inData, outData{};
    /// node executor
    std::unique_ptr<Executor> executor;
    /// owning pointer to widget, may be deleted earlier
    volatile_ptr<QWidget> widget{};
    /// factory for creating the widget
    WidgetFactory widgetFactory{};
    /// node flags
    NodeFlags flags{NoFlag};
    /// iterator for the next port id
    PortId nextPortId{0};

    bool requiresEvaluation{true};

    bool canEvaluate() const
    {
        assert(inData.size() == inPorts.size());

        PortIndex idx{0};
        for (auto const& data : inData)
        {
            auto const& p = inPorts.at(idx++);

            // check if data is required and valid
            if (!p.optional && !data) return false;
        }

        return true;
    }

    std::vector<PortData>& ports(PortType type) noexcept(false)
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

    std::vector<PortData> const& ports(PortType type) const noexcept(false)
    {
        return const_cast<NodeImpl*>(this)->ports(type);
    }

    std::vector<NodeDataPtr>& nodeData(PortType type) noexcept(false)
    {
        switch (type)
        {
        case PortType::In:
            return inData;
        case PortType::Out:
            return outData;
        case PortType::NoType:
            break;
        }

        throw GTlabException{
            __FUNCTION__, QStringLiteral("Invalid port type specified!")
        };
    }

    std::vector<NodeDataPtr> const& nodeData(PortType type) const noexcept(false)
    {
        return const_cast<NodeImpl*>(this)->nodeData(type);
    }

    struct FindData
    {
        PortType type{PortType::NoType};
        PortIndex idx{};
        std::vector<PortData>* ports{};
        std::vector<NodeDataPtr>* data{};

        operator bool() const {
            return ports && data && type != PortType::NoType && idx != invalid<PortIndex>();
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
                auto& data = nodeData(type);
                assert(ports.size() == data.size());
                return {type, PortIndex::fromValue(idx), &ports, &data};
            }
        }
        return {};
    }
};

} // namespace intelli

#endif // GT_INTELLI_NODE_IMPL_H
