/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 24.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPHNODE_IMPL_H
#define GT_INTELLIGRAPHNODE_IMPL_H

#include "gt_igvolatileptr.h"
#include "gt_intproperty.h"
#include "gt_doubleproperty.h"

#include "gt_intelligraphnode.h"
#include "gt_intelligraphexecutor.h"

struct GtIntelliGraphNode::Impl
{
    Impl(QString const& name) : modelName(name) {}

    /// node id
    GtIntProperty id{"id", tr("Node Id"), tr("Node Id")};
    /// x position of node
    GtDoubleProperty posX{"posX", tr("x-Pos"), tr("x-Position")};
    /// y position of node
    GtDoubleProperty posY{"posY", tr("y-Pos"), tr("y-Position")};
    /// width of node widget
    GtIntProperty sizeWidth{"sizeWidth", tr("Size width"), tr("Size width"), GtUnit::NonDimensional, -1};
    /// height of node widget
    GtIntProperty sizeHeight{"sizeHeight", tr("Size height"), tr("Size height"), GtUnit::NonDimensional, -1};
    /// caption string
    QString modelName;
    /// ports
    std::vector<PortData> inPorts, outPorts{};
    /// data
    std::vector<NodeData> inData, outData{};
    /// owning pointer to widget, may be deleted earlier
    gt::ig::volatile_ptr<QWidget> widget{};
    /// factory for creating the widget
    WidgetFactory widgetFactory{};
    /// node flags
    NodeFlags flags{gt::ig::NoFlag};
    /// iterator for the next port id
    PortId nextPortId{0};

    GtIntellIGraphExecutor executor;

    State state{EvalRequired};

    bool active{false};

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
};

#endif // GT_INTELLIGRAPHNODE_IMPL_H
