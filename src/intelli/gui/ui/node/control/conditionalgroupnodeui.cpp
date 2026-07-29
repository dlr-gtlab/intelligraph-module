/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#include "intelli/gui/ui/node/control/conditionalgroupnodeui.h"

#include "intelli/data/double.h"
#include "intelli/node/control/conditional.h"
#include "intelli/private/utils.h"

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

ConditionalGroupNodeUI::ConditionalGroupNodeUI() :
    NodeUI(Option::NoDefaultPortActions)
{

    addSingleAction(tr("Add In Port"), addInPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(
            Op{}.OR(toConditionalNode).OR(toConditionalOutputNode));

    addSingleAction(tr("Add Out Port"), addOutPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(
            Op{}.OR(toConditionalNode).OR(toConditionalInputNode));
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
    auto cmd = gtApp->makeCommand(node.parentObject(),
                                  QStringLiteral("Adding an %1put port to conditional node '%2'")
                                      .arg(type == PortType::In ? "in" : "out",
                                           relativeNodePath(node)));
    Q_UNUSED(cmd);

    // TODO: add option for type id

    auto id = (type == PortType::In) ?
                  node.addDataInPort(typeId<DoubleData>()) :
                  node.addDataOutPort(typeId<DoubleData>());

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
    if (!node) return;

    ::addPort(*node, PortType::In);
}

void
ConditionalGroupNodeUI::addOutPort(GtObject* obj)
{
    auto* node = toConditionalNode(obj);
    if (!node) return;

    ::addPort(*node, PortType::Out);
}

