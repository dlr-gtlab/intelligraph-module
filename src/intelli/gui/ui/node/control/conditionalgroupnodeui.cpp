/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/gui/ui/node/control/conditionalgroupnodeui.h"

#include "intelli/node/control/conditional.h"

#include <gt_icons.h>

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
    GraphUI(CustomOrder)
{
    auto nodeActions = NodeUI::defaultNodeActions();

    nodeActions.remove(ProviderNodeAction);

    nodeActions.insertAfter(
        AddPortNodeAction,
        makeSingleAction(tr("Add In Port"), addInPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toConditionalOutputNode),
        ProviderPortAction);

    nodeActions.insertAfter(
        AddPortNodeAction,
        makeSingleAction(tr("Add Out Port"), addOutPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toConditionalInputNode),
        ProviderNodeAction);

    initializeNodeActions(nodeActions);

    auto portActions = NodeUI::defaultPortActions();

    portActions.remove(ProviderPortAction);

    portActions.insertAfter(
        EditPortAction,
        makePortAction(tr("Edit Port"), editPort)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(toDataPort),
        ProviderPortAction);

    portActions.insertAfter(
        ProviderPortAction,
        makePortAction(tr("Delete Port"), deletePort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(toDataPort));

    initializePortActions(portActions);
}

QIcon
ConditionalGroupNodeUI::displayIcon(Node const& node)  const
{
    if (toConstConditionalNode(&node))
    {
        return gt::gui::icon::objectFreestyleComponent();
    }
    return NodeUI::displayIcon(node);
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

