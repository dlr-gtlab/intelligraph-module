/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODE_IMPL_H
#define GT_INTELLI_NODE_IMPL_H

#include <intelli/node.h>
#include <intelli/memory.h>
#include <intelli/property/uint.h>

#include <gt_intproperty.h>
#include <gt_doubleproperty.h>
#include <gt_boolproperty.h>
#include <gt_exceptions.h>

namespace intelli
{

struct Node::Impl
{
    using PortInfo      = Node::PortInfo;
    using WidgetFactory = Node::WidgetFactory;

    Impl(QString const& name) : modelName(name) { }

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
    /// tooltip
    QString toolTip;
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

    /**
     * @brief Returns an iterator to the port specified by `id`
     * @param ports Port list
     * @param id Port id to search for
     */
    template <typename Ports>
    static inline auto
    find(Ports& ports, PortId id)
    {
        return std::find_if(ports.begin(), ports.end(), [id](auto const& p){
            return p.id() == id;
        });
    }

    /**
     * @brief Returns the ports of the specified type
     * @param type Port type
     * @return Ports
     */
    inline std::vector<PortInfo>&
    ports(PortType type) noexcept(false)
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

    inline std::vector<PortInfo> const&
    ports(PortType type) const noexcept(false)
    {
        return const_cast<Impl*>(this)->ports(type);
    }

    /// Helper struct to provide easy access to the meta data of a port
    struct PortDataHelper
    {
        PortType type{PortType::NoType};
        PortIndex idx{};
        std::vector<PortInfo>* ports{};

        operator bool() const {
            return ports && type != PortType::NoType && idx != invalid<PortIndex>();
        }
    };

    /**
     * @brief Attempts to find the port specified by `id` and returns helper
     * struct to provide easy access to its meta data.
     * @param id Port to search for
     * @return Port helper struct
     */
    inline PortDataHelper
    findPort(PortId id)
    {
        for (auto type : { PortType::In, PortType::Out })
        {
            auto& ports = this->ports(type);
            auto iter = Impl::find(ports, id);

            if (iter != ports.end())
            {
                auto idx = std::distance(ports.begin(), iter);
                return {type, PortIndex::fromValue(idx), &ports};
            }
        }
        return {};
    }

    /**
     * @brief Returns the next, unoccupied port id. The input port id can
     * be used to request a custom port id. If a custom port id was specified,
     * but the port id is already in use, an invalid port id is returned
     * @param id Optional port id, which can be used for a custom port id.
     * @return Nnew port id. Invalid, if operation failed.
     */
    inline PortId
    incNextPortId(PortId id = invalid<PortId>())
    {
        if (id != invalid<PortId>())
        {
            // port already exists
            if (findPort(id)) return PortId{};

            // update next port id
            if (id >= nextPortId) nextPortId = PortId(id + 1);

            return id;
        }
        return nextPortId++;
    }

}; // struct Impl

} // namespace intelli

#endif // GT_INTELLI_NODE_IMPL_H
