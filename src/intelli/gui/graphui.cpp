/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/graphui.h"
#include "intelli/graph.h"
#include "intelli/graphutilities.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/icons.h"

#include <gt_application.h>
#include <gt_icons.h>

using namespace intelli;

GraphInputProvider* toInputProvider(GtObject* obj) { return qobject_cast<GraphInputProvider*>(obj); }

GraphInputProvider* toOutputProvider(GtObject* obj) { return qobject_cast<GraphInputProvider*>(obj); }

bool toProvider(GtObject* obj) { return qobject_cast<AbstractGraphProvider const*>(obj); }

bool isProvider(GtObject const* obj, PortType, PortIndex) { return qobject_cast<AbstractGraphProvider const*>(obj); }

GraphUI::GraphUI(Options options) :
    NodeUI(CustomOrder)
{
    if (!options.testFlag(Option::CustomNodeActionsOrder))
    {
        initializeNodeActions(GraphUI::defaultNodeActions());
    }
    if (!options.testFlag(Option::CustomPortActionsOrder))
    {
        initializePortActions(GraphUI::defaultPortActions());
    }
}

NodeUI::NodeActionList
GraphUI::defaultNodeActions() const
{
    auto nodeActions = NodeUI::defaultNodeActions();

    nodeActions.insertAfter(
        CustomNodeAction,
        makeSingleAction(tr("Clear Graph"), clearGraphNode)
            .setIcon(gt::gui::icon::clear())
            .setVisibilityMethod(toGraph));

    nodeActions.insertAfter(
        CustomNodeAction,
        makeSingleAction(tr("Duplicate Graph"), duplicateGraph)
            .setIcon(gt::gui::icon::duplicate())
            .setVisibilityMethod(toGraph)
            .setShortCut(gtApp->getShortCutSequence("clone")));

    nodeActions.insertAfter(
        CustomNodeAction,
        makeSingleAction(tr("Edit User Variables..."), editUserVariables)
            .setIcon(gt::gui::icon::variable())
            .setVisibilityMethod(isRootGraph));

    /** PROVIDER PORT ACTIONS **/

    nodeActions.insertAfter(
        AddPortNodeAction,
        makeSingleAction(tr("Add In Port"), addOutputProviderPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toOutputProvider),
        ProviderNodeAction);

    nodeActions.insertAfter(
        AddPortNodeAction,
        makeSingleAction(tr("Add Out Port"), addInputProviderPort)
            .setIcon(gt::gui::icon::add())
            .setVisibilityMethod(toInputProvider),
        ProviderNodeAction);

    return nodeActions;
}

NodeUI::PortActionList
GraphUI::defaultPortActions() const
{
    auto portActions = NodeUI::defaultPortActions();

    portActions.insertAfter(
        EditPortAction,
        makePortAction(tr("Edit Port"), editProviderPort)
            .setIcon(gt::gui::icon::rename())
            .setVisibilityMethod(isProvider),
        ProviderPortAction);

    portActions.insertAfter(
        ProviderPortAction,
        makePortAction(tr("Delete Port"), deleteProviderPort)
            .setIcon(gt::gui::icon::delete_())
            .setVisibilityMethod(isProvider));

    return portActions;
}

QIcon
GraphUI::displayIcon(const Node& node) const
{
    if (toConstGraph(&node))
    {
        return gt::gui::icon::intelli::intelliGraph();
    }
    if (qobject_cast<GraphInputProvider const*>(&node))
    {
        return gt::gui::icon::import();
    }
    if (qobject_cast<GraphOutputProvider const*>(&node))
    {
        return gt::gui::icon::export_();
    }
    return NodeUI::displayIcon(node);
}

QStringList
GraphUI::openWith(GtObject* obj)
{
    QStringList list = NodeUI::openWith(obj);

    if (toGraph(obj))
    {
        list << GT_CLASSNAME(GraphEditor);
    }

    return list;
}

void
GraphUI::clearGraphNode(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->makeCommand(graph, QStringLiteral("Clear '%1'")
                                             .arg(graph->objectName()));
    Q_UNUSED(cmd);

    graph->clearGraph();
}

void
GraphUI::duplicateGraph(GtObject* obj)
{
    Graph* graph = toGraph(obj);
    if (!graph) return;

    GtObject* parent = graph->parentObject();

    auto cmd = gtApp->makeCommand(parent,
                                  tr("Duplicate graph '%1'")
                                      .arg(relativeNodePath(*graph)));
    Q_UNUSED(cmd);

    utils::duplicateGraph(*graph);
}

void
GraphUI::addInputProviderPort(GtObject* obj)
{
    auto* provider = qobject_cast<GraphInputProvider*>(obj);
    if (!provider) return;

    auto* graph = Graph::accessGraph(*provider);
    if (!graph) return;

    addDynamicInPort(graph);
}

void
GraphUI::addOutputProviderPort(GtObject* obj)
{
    auto* provider = qobject_cast<GraphOutputProvider*>(obj);
    if (!provider) return;

    auto* graph = Graph::accessGraph(*provider);
    if (!graph) return;

    addDynamicOutPort(graph);
}

void
GraphUI::editProviderPort(Node* obj, PortType type, PortIndex idx)
{
    auto* provider = qobject_cast<AbstractGraphProvider*>(obj);
    if (!provider) return;

    auto* graph = Graph::accessGraph(*provider);
    if (!graph) return;

    editDynamicPort(graph, invert(type), idx);
}

void
GraphUI::deleteProviderPort(Node* obj, PortType type, PortIndex idx)
{
    auto* provider = qobject_cast<AbstractGraphProvider*>(obj);
    if (!provider) return;

    auto* graph = Graph::accessGraph(*provider);
    if (!graph) return;

    deleteDynamicPort(graph, invert(type), idx);
}
