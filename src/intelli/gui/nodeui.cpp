/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 14.7.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/nodeui.h"

#include "intelli/dynamicnode.h"
#include "intelli/node.h"
#include "intelli/graph.h"
#include "intelli/graphexecmodel.h"
#include "intelli/data/double.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/icons.h"
#include "intelli/gui/style.h"

#include <gt_logging.h>
#include <gt_colors.h>

#include <gt_command.h>
#include <gt_inputdialog.h>
#include <gt_application.h>

#include <QPainter>
#include <QFileInfo>
#include <QFile>


using BoolObjectMethod = std::function<bool (GtObject*)>;

template <typename Functor>
inline BoolObjectMethod NOT(Functor fA)
{
    return [a = std::move(fA)](GtObject* obj){
        return !a(obj);
    };
}
template <typename Functor>
inline BoolObjectMethod operator+(BoolObjectMethod fA, Functor fOther)
{
    return [a = std::move(fA), b = std::move(fOther)](GtObject* obj){
        return a(obj) && b(obj);
    };
}

using namespace intelli;

NodeUI::NodeUI(Option option)
{
    setObjectName(QStringLiteral("IntelliGraphNodeUI"));

    if ((option & NoDefaultActions)) return;

    auto const isActive = [](GtObject* obj){
        return static_cast<Node*>(obj)->isActive();
    };

    static auto const& categroy =  QStringLiteral("GtProcessDock");

    addSingleAction(tr("Execute once"), executeNode)
        .setIcon(gt::gui::icon::processRun())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("runProcess"), categroy))
        .setVisibilityMethod(toNode);

    addSingleAction(tr("Set inactive"), setActive<false>)
        .setIcon(gt::gui::icon::sleep())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("skipProcess"), categroy))
        .setVisibilityMethod(toNode + isActive);

    addSingleAction(tr("set active"), setActive<true>)
        .setIcon(gt::gui::icon::sleepOff())
        .setShortCut(gtApp->getShortCutSequence(QStringLiteral("unskipProcess"), categroy))
        .setVisibilityMethod(toNode + NOT(isActive));

    addSeparator();

    addSingleAction(tr("Rename Node"), renameNode)
        .setIcon(gt::gui::icon::rename())
        .setVisibilityMethod(toNode)
        .setVerificationMethod(canRenameNodeObject)
        .setShortCut(gtApp->getShortCutSequence("rename"));

    addSingleAction(tr("Clear Intelli Graph"), clearNodeGraph)
        .setIcon(gt::gui::icon::clear())
        .setVisibilityMethod(toGraph);

    if ((option & NoDefaultPortActions)) return;

    auto const hasOutputPorts = [](GtObject* obj){
        return !(static_cast<DynamicNode*>(obj)->dynamicNodeOption() &
                 DynamicNode::DynamicInputOnly);
    };
    auto const hasInputPorts = [](GtObject* obj){
        return !(static_cast<DynamicNode*>(obj)->dynamicNodeOption() &
                 DynamicNode::DynamicOutputOnly);
    };

    addSeparator();

    addSingleAction(tr("Add In Port"), addInPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toDynamicNode + hasInputPorts);

    addSingleAction(tr("Add Out Port"), addOutPort)
        .setIcon(gt::gui::icon::add())
        .setVisibilityMethod(toDynamicNode + hasOutputPorts);

    /** PORT ACTIONS **/

    addPortAction(tr("Delete Port"), deleteDynamicPort)
        .setIcon(gt::gui::icon::delete_())
        .setVerificationMethod(isDynamicPort)
        .setVisibilityMethod(isDynamicNode);

    if (!gtApp || !gtApp->devMode()) return;

    addSingleAction(tr("Node Info"), [](GtObject* obj){
        auto* node = toNode(obj);
        if (!node) return;
        auto* graph = obj->findParent<Graph*>();
        if (!graph) return;
        auto* model = graph->executionModel();
        if (!model) return;
        model->debug(node->id());
    }).setIcon(gt::gui::icon::bug());

//    addSingleAction(tr("Update Node"), [](GtObject* obj){
//        auto* node = toNode(obj);
//        if (!node) return;
//        emit node->nodeStateChanged();
//    }).setIcon(gt::gui::icon::bug());

    addPortAction(tr("Port Info"), [](Node* obj, PortType type, PortIndex idx){
        if (!obj) return;
        gtInfo() << tr("Node '%1' (id: %2), Port id: %3")
                        .arg(obj->caption())
                        .arg(obj->id())
                        .arg(obj->portId(type, idx));
    }).setIcon(gt::gui::icon::bug());
}

QColor
NodeUI::backgroundColor(Node& node) const
{
    QColor const& bg = style::nodeBackground();

    if (node.nodeFlags() & NodeFlag::Unique)
    {
        return gt::gui::color::lighten(bg, 20);
    }
    if (toGraph(&node))
    {
        return gt::gui::color::lighten(bg, 10);
    }

    return bg;
}

Painter
NodeUI::painter(NodeGraphicsObject& object, QPainter& painter) const
{
    return {object, painter, geometry(*object.node())};
}

Geometry
NodeUI::geometry(Node& node) const
{
    return {node};
}

QIcon
NodeUI::icon(GtObject* obj) const
{
    if (toGraph(obj))
    {
        return gt::gui::icon::intelli::intelliGraph();
    }

    return gt::gui::icon::intelli::node();
}

QStringList
NodeUI::openWith(GtObject* obj)
{
    QStringList list;

    if (toGraph(obj))
    {
        list << GT_CLASSNAME(GraphEditor);
    }

    return list;
}

PortUIAction&
NodeUI::addPortAction(const QString& actionText,
                                    PortActionFunction actionMethod)
{
    m_portActions.append(PortUIAction(actionText, std::move(actionMethod)));
    return m_portActions.back();
}

Graph*
NodeUI::toGraph(GtObject* obj)
{
    return qobject_cast<Graph*>(obj);
}

Node*
NodeUI::toNode(GtObject* obj)
{
    return qobject_cast<Node*>(obj);
}

DynamicNode*
NodeUI::toDynamicNode(GtObject* obj)
{
    return qobject_cast<DynamicNode*>(obj);;
}

bool
NodeUI::isDynamicPort(GtObject* obj, PortType type, PortIndex idx)
{
    if (auto* node = toDynamicNode(obj))
    {
        return node->isDynamicPort(type, idx);
    }
    return false;
}

bool
NodeUI::isDynamicNode(GtObject* obj, PortType, PortIndex)
{
    return toDynamicNode(obj);
}

bool
NodeUI::canRenameNodeObject(GtObject* obj)
{
    if (auto* node = toNode(obj))
    {
        return !(node->nodeFlags() & Unique);
    }
    return true;
}

void
NodeUI::renameNode(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node) return;

    GtInputDialog dialog(GtInputDialog::TextInput);
    dialog.setWindowTitle(tr("Rename Node Object"));
    dialog.setWindowIcon(gt::gui::icon::rename());
    dialog.setLabelText(tr("Enter the new node base name."));
    dialog.setInitialTextValue(node->baseObjectName());

    if (dialog.exec())
    {
        auto text = dialog.textValue();
        if (!text.isEmpty())
        {
            node->setCaption(text);
        }
    }
}

void
NodeUI::executeNode(GtObject* obj)
{
    auto* node = toNode(obj);
    if (!node) return;

    auto* graph = toGraph(node->parentObject());
    if (!graph) return;

    auto cleanup = gt::finally([node, old = node->isActive()](){
        node->setActive(old);
    });
    Q_UNUSED(cleanup);
    
    auto* model = graph->makeExecutionModel();

    node->setActive();
    model->invalidateNode(node->id());
    model->evaluateNode(node->id()).detach();
}

void
NodeUI::addInPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addInPort(typeId<DoubleData>());
    gtInfo().verbose() << tr("Added dynamic in port with id") << id;
}

void
NodeUI::addOutPort(GtObject* obj)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    auto id = node->addOutPort(typeId<DoubleData>());
    gtInfo().verbose() << tr("Added dynamic out port with id") << id;
}

void
NodeUI::deleteDynamicPort(Node* obj, PortType type, PortIndex idx)
{
    auto* node = toDynamicNode(obj);
    if (!node) return;

    node->removePort(node->portId(type, idx));
}

void
NodeUI::clearNodeGraph(GtObject* obj)
{
    auto graph = toGraph(obj);
    if (!graph) return;

    auto cmd = gtApp->startCommand(graph, QStringLiteral("Clear '%1'")
                                              .arg(graph->objectName()));
    auto finally = gt::finally([&](){ gtApp->endCommand(cmd); });
    
    graph->clearGraph();
}

void
NodeUI::setActive(GtObject* obj, bool state)
{
    auto* node = toNode(obj);
    if (!node) return;

    auto wasActive = node->isActive();

    node->setActive(state);

    if (!wasActive && node->isActive())
    {
        emit node->triggerNodeEvaluation();
    }
}

constexpr double s_port_diameter = 8.0;
constexpr double s_rounded_rect_radius = 2.0;

Geometry::Geometry(Node& node) :
    m_node(&node)
{

}

double
Geometry::hspacing() const
{
    return 10;
}

double
Geometry::vspacing() const
{
    return hspacing();
}

QRectF
Geometry::innerRect() const
{
    auto rect = boundingRect();

    return QRectF(rect.topLeft() + QPointF{s_port_diameter * 0.5, 0},
                  QSizeF{rect.width() - s_port_diameter, rect.height()});
}

QRectF
Geometry::boundingRect() const
{
    return QRectF(QPoint{0, 0}, QSize{200, 200});
}

QRectF
Geometry::captionRect() const
{
    QFont f;
    f.setBold(true);
    QFontMetrics boldFontMetrics(f);

    return boldFontMetrics.boundingRect(m_node->caption());
}

QPointF
Geometry::evalStateVisualizerPosition() const
{
    return boundingRect().topLeft() + QPointF{s_port_diameter * 0.5, 0};
}

QRectF
Geometry::portRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    double height = 0.0;

    height += captionRect().height();
    height += 1.0 * vspacing(); // below caption

    for (PortIndex i{0}; i < idx; ++i)
    {
        auto* port = m_node->port(m_node->portId(type, i));
        assert(port);

        height += vspacing() * (0.5 + 1 + port->caption.count('\n'));
    }

    height += vspacing();

    double width = type == PortType::Out ? innerRect().width() : 0.0;

    return {QPointF(width, height), QSizeF{s_port_diameter, s_port_diameter}};
}

QRectF
Geometry::resizeHandleRect() const
{
    constexpr QSize size{10, 10};

    QRectF rect = innerRect();
    return QRectF(rect.bottomRight() - QPoint{size.width(), size.height()}, size);
}

QPointF
Geometry::widgetPosition() const
{
    return {};
}

Geometry::PortHit
Geometry::portHit(QPointF coord) const
{
    return {};
}

void
Geometry::recomputeGeomtry()
{

}

Painter::Painter(NodeGraphicsObject& obj, QPainter& painter, Geometry geometry) :
    m_object(&obj), m_painter(&painter), m_geometry(geometry)
{
    paint();
}

void
Painter::drawRect()
{

    auto color = m_object->isSelected() ? style::boundarySelected() : style::boundaryDefault();

    QPen pen(color, m_object->isHovered() ? style::borderWidthHovered() : style::borderWidthDefault());
    m_painter->setPen(pen);
    m_painter->setBrush(style::nodeBackground());

    m_painter->drawRoundedRect(m_geometry.innerRect(),
                               s_rounded_rect_radius,
                               s_rounded_rect_radius);
}

void
Painter::drawPorts()
{
    auto* node = m_object->node();
    assert(node);

    for (PortType portType : {PortType::Out, PortType::In})
    {
        size_t const n = node->ports(portType).size();

        for (PortIndex portIndex{0}; portIndex < n; ++portIndex)
        {
            QRectF p = m_geometry.portRect(portType, portIndex);

//            double r = 1.0;

//            if (auto const *cgo = state.connectionForReaction())
//            {
//                PortType requiredPort = cgo->connectionState().requiredPort();

//                if (requiredPort == portType) {
//                    ConnectionId possibleConnectionId = makeCompleteConnectionId(cgo->connectionId(),
//                                                                                 nodeId,
//                                                                                 portIndex);

//                    bool const possible = model.connectionPossible(possibleConnectionId);

//                    auto cp = cgo->sceneTransform().map(cgo->endPoint(requiredPort));
//                    cp = ngo.sceneTransform().inverted().map(cp);

//                    auto diff = cp - p;
//                    double dist = std::sqrt(QPointF::dotProduct(diff, diff));

//                    if (possible) {
//                        double const thres = 40.0;
//                        r = (dist < thres) ? (2.0 - dist / thres) : 1.0;
//                    } else {
//                        double const thres = 80.0;
//                        r = (dist < thres) ? (dist / thres) : 1.0;
//                    }
//                }
//            }

            m_painter->setBrush(Qt::red);
            m_painter->drawEllipse(p);
        }
    }

//    if (ngo.nodeState().connectionForReaction())
//    {
//        ngo.nodeState().resetConnectionForReaction();
//    }
}

void
Painter::drawCaption()
{
    auto* node = m_object->node();
    assert(node);

    if (node->nodeFlags() & NodeFlag::HideCaption) return;

    m_painter->save();

    QFont f = m_painter->font();
    f.setBold(true);

    QPointF pos = m_geometry.captionRect().topLeft();

    m_painter->setFont(f);
    m_painter->setPen(gt::gui::color::text());
    m_painter->drawText(pos, node->caption());

    m_painter->restore();
}

void
Painter::drawResizeRect()
{
    auto* node = m_object->node();
    assert(node);

    if (node->nodeFlags() & NodeFlag::Resizable)
    {
        m_painter->setBrush(Qt::gray);
        m_painter->drawEllipse(m_geometry.resizeHandleRect());
    }
}

void
Painter::paint()
{
    drawRect();
    drawPorts();
    drawCaption();
    drawResizeRect();
}
