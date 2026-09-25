/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/gui/ui/node/control/conditionalgroupnodeui.h"

#include "intelli/node/control/conditional.h"

#include <gt_icons.h>

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
 #define ORDER_PRIORITY(X) X
 #define SET_ORDER_PRIORITY(X) .setOrderPriority(X)
#else
 #define ORDER_PRIORITY(X) void
 #define SET_ORDER_PRIORITY(X)
#endif

using namespace intelli;

using BoolObjectMethod = std::function<bool (GtObject*)>;

ConditionalGroupNode* toConditionalNode(GtObject* obj)
{
    return qobject_cast<ConditionalGroupNode*>(obj);
}

ConditionalGroupNode const* toConstConditionalNode(GtObject const* obj)
{
    return qobject_cast<ConditionalGroupNode const*>(obj);
}

ConditionalInputProvider* toConditionalInputNode(GtObject* obj)
{
    return qobject_cast<ConditionalInputProvider*>(obj);
}

ConditionalOutputProvider* toConditionalOutputNode(GtObject* obj)
{
    return qobject_cast<ConditionalOutputProvider*>(obj);
}

Node::PortInfo*
toDataPort(Node* obj, PortType type, PortIndex idx)
{
    auto node = toConditionalNode(obj);
    if (!node)
    {
        if (toConditionalInputNode(obj))
        {
            node = toConditionalNode(obj->parentObject());
            type = invert(type);
            idx++;
        }
        if (toConditionalOutputNode(obj))
        {
            node = toConditionalNode(obj->parentObject());
            type = invert(type);
        }
        if (!node) return nullptr;
    }

    PortId portId = node->portId(type, idx);
    if (!node->isDataPort(portId)) return nullptr;

    return node->port(portId);
}

ConditionalGroupNodeUI::ConditionalGroupNodeUI() :
    GraphUI(NoProviderActions)
{
    addSingleAction(tr("Add In Port"), addInPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toConditionalOutputNode)
        SET_ORDER_PRIORITY(OrderPriority::PortAction);

    addSingleAction(tr("Add Out Port"), addOutPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toConditionalInputNode)
        SET_ORDER_PRIORITY(OrderPriority::PortAction);

    // port actions

    addPortAction(tr("Edit Port"), editPort)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(toDataPort);

    addPortAction(tr("Delete Port"), deletePort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(toDataPort);
}

QIcon
ConditionalGroupNodeUI::displayIcon(Node const& node)  const
{
    if (toConstConditionalNode(&node))
    {
        return gt::gui::icon::objectFreestyleComponent();
    }
    return GraphUI::displayIcon(node);
}

void
ConditionalGroupNodeUI::addInPort(GtObject* obj)
{
    auto* node = toConditionalOutputNode(obj);
    if (!node) return;

    addDynamicOutPort(Graph::accessGraph(*node));
}

void
ConditionalGroupNodeUI::addOutPort(GtObject* obj)
{
    auto* node = toConditionalInputNode(obj);
    if (!node) return;

    addDynamicInPort(Graph::accessGraph(*node));
}

void
ConditionalGroupNodeUI::editPort(Node* obj, PortType type, PortIndex idx)
{
    if (toConditionalInputNode(obj))
    {
        return editDynamicPort(Graph::accessGraph(*static_cast<Node*>(obj)), invert(type), ++idx);
    }
    if (toConditionalOutputNode(obj))
    {
        return editDynamicPort(Graph::accessGraph(*static_cast<Node*>(obj)), invert(type), idx);
    }
}

void
ConditionalGroupNodeUI::deletePort(Node* obj, PortType type, PortIndex idx)
{
    if (toConditionalInputNode(obj))
    {
        return deleteDynamicPort(Graph::accessGraph(*static_cast<Node*>(obj)), invert(type), ++idx);
    }
    if (toConditionalOutputNode(obj))
    {
        return deleteDynamicPort(Graph::accessGraph(*static_cast<Node*>(obj)), invert(type), idx);
    }
}

