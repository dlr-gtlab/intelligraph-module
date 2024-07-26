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

using namespace intelli::lib;

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
    using WidgetFactory = Node::WidgetFactory;

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

    /// factory for creating the widget
    WidgetFactory widgetFactory{};

    NodeDataInterface* dataInterface{};

};

} // namespace intelli

#endif // GT_INTELLI_NODE_IMPL_H
