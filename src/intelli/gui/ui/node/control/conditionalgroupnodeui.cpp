/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/gui/ui/node/control/conditionalgroupnodeui.h"

#include "intelli/data/double.h"
#include "intelli/data/int.h"
#include "intelli/node/control/conditional.h"
#include "intelli/private/utils.h"
#include "intelli/gui/widgets/porteditdialog.h"

#include <gt_icons.h>

using namespace intelli;

using BoolObjectMethod = std::function<bool (GtObject*)>;

struct Op
{
    BoolObjectMethod f = nullptr;

    operator BoolObjectMethod() const { assert(f); return f; }

    template <typename Functor>
    Op& OR(Functor fOther)
    {
        f = [a = std::move(f), b = std::move(fOther)](GtObject* obj){
            return (a && a(obj)) || b(obj);
        };
        return *this;
    }
    template <typename Functor>
    Op& OR_NOT(Functor fOther)
    {
        f = [a = std::move(f), b = std::move(fOther)](GtObject* obj){
            return (a && a(obj)) || !b(obj);
        };
        return *this;
    }

    template <typename Functor>
    Op& AND(Functor fOther)
    {
        f = [a = std::move(f), b = std::move(fOther)](GtObject* obj){
            return (!a || a(obj)) && b(obj);
        };
        return *this;
    }
    template <typename Functor>
    Op& AND_NOT(Functor fOther)
    {
        f = [a = std::move(f), b = std::move(fOther)](GtObject* obj){
            return (!a || a(obj)) && !b(obj);
        };
        return *this;
    }
};

ConditionalGroupNode* toConditionalNode(GtObject* obj)
{
    return qobject_cast<ConditionalGroupNode*>(obj);
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
    NodeUI(/*Option::NoDefaultPortActions*/)
{

//    addSingleAction(tr("Add In Port"), addInPort)
//        .setIcon(gt::gui::icon::add())
//        .setVisibilityMethod(
//            Op{}.OR(toConditionalNode).OR(toConditionalOutputNode));

//    addSingleAction(tr("Add Out Port"), addOutPort)
//        .setIcon(gt::gui::icon::add())
//        .setVisibilityMethod(
//            Op{}.OR(toConditionalNode).OR(toConditionalInputNode));

//    addPortAction(tr("Edit Port"), editPort)
//        .setIcon(gt::gui::icon::rename())
//        .setVisibilityMethod(toDataPort);

//    addPortAction(tr("Delete Port"), deletePort)
//        .setIcon(gt::gui::icon::delete_())
//        .setVisibilityMethod(toDataPort);
}

QIcon
ConditionalGroupNodeUI::icon(GtObject* obj) const
{
    if (toConditionalNode(obj))
    {
        return gt::gui::icon::objectFreestyleComponent();
    }
    return NodeUI::icon(obj);
}

namespace
{

void
addPort(ConditionalGroupNode& node, PortType type)
{
    assert(node.parentObject());

    PortEditDialog dialog{type};
    if (!dialog.exec()) return;

    Node::PortInfo portInfo{dialog.typeId()};
    portInfo.caption = dialog.caption();
    portInfo.captionVisible = dialog.captionVisible();

    // TODO: undo/redo command not working, since multiple nodes are updated in parallel
    auto cmd = gtApp->makeCommand(&node,
                                  QStringLiteral("Adding an %1put port to conditional node '%2'")
                                      .arg(type == PortType::In ? "in" : "out",
                                           relativeNodePath(node)));
    Q_UNUSED(cmd);

    auto id = (type == PortType::In) ?
                  node.addDataInPort(std::move(portInfo)) :
                  node.addDataOutPort(std::move(portInfo));

    auto* port = node.port(id);
    if (!port)
    {
        gtWarning().verbose() << QObject::tr("Failed to add dynamic port to %1!")
                                  .arg(relativeNodePath(node));
        return;
    }
    gtInfo().verbose() << QObject::tr("Added dynamic port '%1'")
                              .arg(port ? toString(*port) : "N/A");
}

} // namespace

void
ConditionalGroupNodeUI::addInPort(GtObject* obj)
{
    auto* node = toConditionalNode(obj);
    if (!node)
    {
        if (toConditionalOutputNode(obj))
        {
            return addOutPort(obj->parentObject());
        }
        return;
    }

    ::addPort(*node, PortType::In);
}

void
ConditionalGroupNodeUI::addOutPort(GtObject* obj)
{
    auto* node = toConditionalNode(obj);
    if (!node)
    {
        if (toConditionalInputNode(obj))
        {
            return addInPort(obj->parentObject());
        }
        return;
    }

    ::addPort(*node, PortType::Out);
}

void
ConditionalGroupNodeUI::editPort(Node* obj, PortType type, PortIndex idx)
{
    auto* node = toConditionalNode(obj);
    if (!node)
    {
        if (toConditionalInputNode(obj) || toConditionalOutputNode(obj))
        {
            node = toConditionalNode(obj->parentObject());
        }
        if (!node) return;
    }

    auto* srcPort = toDataPort(obj, type, idx);
    if (!srcPort) return;

    PortEditDialog dialog{type};
    dialog.setTypeId(srcPort->typeId);
    dialog.setCaption(srcPort->caption);
    dialog.setCaptionVisible(srcPort->captionVisible);
    if (!dialog.exec()) return;

    // TODO: undo/redo command not working, since multiple nodes are updated in parallel
    auto cmd = gtApp->makeCommand(node,
                                  QStringLiteral("Edited port '%1' of node '%2'")
                                      .arg(toString(*srcPort), relativeNodePath(*node)));
    Q_UNUSED(cmd);

    auto port = *srcPort;
    port.typeId = dialog.typeId();
    port.caption = dialog.caption();
    port.captionVisible = dialog.captionVisible();
    node->updateDataPort(srcPort->id(), port);
}

void
ConditionalGroupNodeUI::deletePort(Node* obj, PortType type, PortIndex idx)
{
    auto* node = toConditionalNode(obj);
    if (!node)
    {
        if (toConditionalInputNode(obj) || toConditionalOutputNode(obj))
        {
            node = toConditionalNode(obj->parentObject());
        }
        if (!node) return;
    }

    auto* port = toDataPort(obj, type, idx);
    if (!port) return;

    // TODO: undo/redo command not working, since multiple nodes are updated in parallel
    auto cmd = gtApp->makeCommand(node,
                                  QStringLiteral("Deleting port '%1' of node '%2'")
                                      .arg(toString(*port), relativeNodePath(*node)));
    Q_UNUSED(cmd);

    node->removeDataPort(port->id());
}

