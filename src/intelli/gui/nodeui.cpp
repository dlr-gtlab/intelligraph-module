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
#include "intelli/nodedatafactory.h"
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

std::unique_ptr<NodePainter>
NodeUI::painter(NodeGraphicsObject& object, NodeGeometry& geometry) const
{
    return std::make_unique<NodePainter>(object, geometry);
}

std::unique_ptr<NodeGeometry>
NodeUI::geometry(Node& node) const
{
    return std::make_unique<NodeGeometry>(node);
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

constexpr int s_port_diameter = 8;
constexpr int s_rounded_rect_radius = 2;
constexpr int s_eval_state_width = 20;

NodeGeometry::NodeGeometry(Node& node) :
    m_node(&node)
{

}

bool
NodeGeometry::positionWidgetAtBottom() const
{
    return m_node->nodeFlags() & NodeFlag::Resizable && m_node->embeddedWidget();
}

int
NodeGeometry::hspacing() const
{
    return 10;
}

int
NodeGeometry::vspacing() const
{
    return hspacing();
}

int
NodeGeometry::captionHeightExtend() const
{
    QRectF caption = captionRect();
    return caption.height() + 2 * caption.topLeft().y();
}

int
NodeGeometry::portHorizontalExtend(PortType type) const
{
    int advance = 0;

    auto n = m_node->ports(type).size();
    for (PortIndex idx{0}; idx < n; ++idx)
    {
        advance = std::max(advance, (int)portCaptionRect(type, idx).width() + hspacing());
    }
    return advance;
}

int
NodeGeometry::portHeightExtend() const
{
    QRectF caption = captionRect();
    int height = caption.height() + caption.topLeft().y();

    for (PortType type : {PortType::In, PortType::Out})
    {
        auto n = m_node->ports(type).size();
        if (n == 0) continue;

        height = std::max(height, (int)(portCaptionRect(type, PortIndex::fromValue(n - 1)).bottomLeft().y() + 0.5 * vspacing()));
    }

    return height;
}

QPainterPath
NodeGeometry::shape() const
{
    auto rect = innerRect();

    rect = QRectF(rect.topLeft() - QPointF{s_port_diameter * 0.5, 0},
                  QSizeF{rect.width() + s_port_diameter, rect.height()});

    QPainterPath path;
    path.addRect(rect);
    return path;
}

QRectF
NodeGeometry::innerRect() const
{
    // some functions may require inner rect to calculate their actual position
    // -> return empty rect to avoid cyclic calls
    if (m_isCalculating) return {};

    m_isCalculating = true;
    auto cleanup = gt::finally([this](){ m_isCalculating = false; });

    QSize wSize{0, 0};
    if (auto w = m_node->embeddedWidget())
    {
        wSize = w->size();
    }
    bool positionAtBottom = positionWidgetAtBottom();

    // height
    int height = 0.5 * vspacing() + portHeightExtend();

    // width
    int width = 0;
    for (PortType type : {PortType::In, PortType::Out})
    {
        width += portHorizontalExtend(type) + hspacing();
    }

    if (positionAtBottom)
    {
        height += wSize.height(); //+ (m_node->nodeFlags() & NodeFlag::Resizable) * resizeHandleRect().height();
        width   = std::max(width, (int)wSize.width() + hspacing());
    }
    else
    {
        height = std::max((double)height, captionHeightExtend() + wSize.height() + 0.5 * vspacing());
        width += wSize.width();
    }

    width = std::max(width, (int)(s_eval_state_width + hspacing() + captionRect().width()));

    return QRectF(QPoint{0, 0}, QSize{width, height});
}

QRectF
NodeGeometry::boundingRect() const
{
    auto rect = innerRect();

    return QRectF(rect.topLeft() - QPointF{s_port_diameter * 2.5, s_port_diameter * 2.5},
                  QSizeF{rect.width() + s_port_diameter * 5, rect.height() + s_port_diameter * 5});
}

QRectF
NodeGeometry::captionRect() const
{
    QFont f;
    f.setBold(true);
    QFontMetrics metrics{f};

    // center caption
    int width = metrics.horizontalAdvance(m_node->caption());
    width += 2;
    QRectF rect{innerRect().topLeft(), QSize{width, metrics.height()}};
    double offset = 0.5 * (innerRect().width() - rect.width()) + 0.5 * s_eval_state_width;
    rect.translate(offset, 0.5 * vspacing());

    return rect;
}

QPointF
NodeGeometry::evalStateVisualizerPosition() const
{
    return innerRect().topLeft();
}

QPointF
NodeGeometry::widgetPosition() const
{
    auto* w = m_node->embeddedWidget();
    if (!w) return {};

    if (positionWidgetAtBottom())
    {
        double xOffset = 0.5 * (innerRect().width() - w->width());
        double yOffset = portHeightExtend();

        return QPointF{xOffset, yOffset};
    }

    double xOffset = hspacing() + portHorizontalExtend(PortType::In);
    double yOffset = captionHeightExtend();

    if (m_node->ports(PortType::In).empty())
    {
        xOffset = innerRect().width() - w->width() - hspacing() - portHorizontalExtend(PortType::Out);
    }

    return QPointF{xOffset, yOffset};
}

QRectF
NodeGeometry::portRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    QFontMetrics metrics{QFont()};

    double height = captionHeightExtend() + 0.5 * vspacing();

    for (PortIndex i{0}; i < idx; ++i)
    {
        height += 0.5 * vspacing() + metrics.height();
    }

    double width = type == PortType::Out ? innerRect().width() : 0.0;
    width -= 0.5 * s_port_diameter;

    return {QPointF(width, height), QSizeF{s_port_diameter, s_port_diameter}};
}

QRectF
NodeGeometry::portCaptionRect(PortType type, PortIndex idx) const
{
    assert(type != PortType::NoType && idx != invalid<PortIndex>());

    QFontMetrics metrics{QFont()};

    auto& factory = NodeDataFactory::instance();

    auto* port = m_node->port(m_node->portId(type, idx));
    assert(port);

    int lineHeight = metrics.height();
    int width = metrics.horizontalAdvance(port->caption.isEmpty() ? factory.typeName(port->typeId) : port->caption);
    width += (width & 1);

    QPointF pos = portRect(type, idx).center();
    pos.setY(pos.y() - lineHeight * 0.5);
    pos.setX(type == PortType::In ?
                 pos.x() + hspacing() :
                 pos.x() - hspacing() - width);

    return QRectF{pos, QSize{width, lineHeight}};
}

NodeGeometry::PortHit
NodeGeometry::portHit(QPointF coord) const
{
    auto rect = innerRect();

    // estimate whether its a input or output port
    PortType type = (coord.x() < rect.x() + 0.5 * rect.width()) ? PortType::In : PortType::Out;

    // check each port
    for (auto& port : m_node->ports(type))
    {
        auto pRect = this->portRect(type, m_node->portIndex(type, port.id()));
        if (pRect.contains(coord))
        {
            return PortHit{type, port.id()};
        }
    }

    return {};
}

QRectF
NodeGeometry::resizeHandleRect() const
{
    constexpr QSize size{8, 8};

    QRectF rect = innerRect();
    return QRectF(rect.bottomRight() - QPoint{size.width(), size.height()}, size);
}

void
NodeGeometry::recomputeGeomtry()
{

}

//////////////////////////////////////////////////

NodePainter::NodePainter(NodeGraphicsObject& obj, NodeGeometry& geometry) :
    m_object(&obj), m_geometry(&geometry)
{

}

QColor
NodePainter::backgroundColor() const
{
    auto& node = m_object->node();

    QColor const& bg = style::nodeBackground();

    if (node.nodeFlags() & NodeFlag::Unique)
    {
        return gt::gui::color::lighten(bg, 20);
    }
    if (NodeUI::toGraph(&node))
    {
        return gt::gui::color::lighten(bg, 10);
    }

    return bg;
}

void
NodePainter::drawRect(QPainter& painter)
{
    // draw backgrond
    painter.setPen(Qt::NoPen);
    painter.setBrush(style::nodeBackground());

    auto rect = m_geometry->innerRect();

    painter.drawRoundedRect(rect,
                            s_rounded_rect_radius,
                            s_rounded_rect_radius);

    // draw resize rect
    auto& node = m_object->node();
    if (node.nodeFlags() & NodeFlag::Resizable && node.embeddedWidget())
    {
        QRectF rect = m_geometry->resizeHandleRect();
        QPolygonF poly;
        poly.append(rect.bottomLeft());
        poly.append(rect.bottomRight());
        poly.append(rect.topRight());

        painter.setPen(Qt::NoPen);
        painter.setBrush(gt::gui::color::lighten(style::boundaryDefault(), -30));
        painter.drawPolygon(poly);
    }

    // draw border
    QColor color = m_object->isSelected() ? style::boundarySelected() : style::boundaryDefault();
    QPen pen(color, m_object->isHovered() ? style::borderWidthHovered() : style::borderWidthDefault());
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawRoundedRect(rect,
                            s_rounded_rect_radius,
                            s_rounded_rect_radius);
}

void
NodePainter::drawPorts(QPainter& painter)
{
    auto& factory = NodeDataFactory::instance();

    auto& node = m_object->node();
    auto& graph = m_object->graph();

    for (PortType type : {PortType::Out, PortType::In})
    {
        size_t const n = node.ports(type).size();

        for (PortIndex idx{0}; idx < n; ++idx)
        {
            auto* port = node.port(node.portId(type, idx));
            assert(port);

            if (!port->visible) continue;

            QRectF p = m_geometry->portRect(type, idx);

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

            QColor color = m_object->isSelected() ? style::boundarySelected() : style::boundaryDefault();
            QPen pen(color, m_object->isHovered() ? style::borderWidthHovered() : style::borderWidthDefault());
            painter.setPen(pen);
            painter.setBrush(gt::gui::color::disabled());
            painter.drawEllipse(p);

            if (!port->captionVisible) continue;

            bool connected = !graph.findConnections(node.id(), port->id()).empty();

            painter.setPen(connected ? gt::gui::color::text() : gt::gui::color::disabled());

            painter.drawText(m_geometry->portCaptionRect(type, idx),
                                port->caption.isEmpty() ? factory.typeName(port->typeId) : port->caption,
                                type == PortType::In ? QTextOption{Qt::AlignLeft} : QTextOption{Qt::AlignRight});

            continue;
            painter.setPen(Qt::yellow);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(m_geometry->portRect(type, idx));
            painter.drawRect(m_geometry->portCaptionRect(type, idx));
        }
    }

//    if (ngo.nodeState().connectionForReaction())
//    {
//        ngo.nodeState().resetConnectionForReaction();
//    }
}

void
NodePainter::drawCaption(QPainter& painter)
{
    auto& node = m_object->node();

    if (node.nodeFlags() & NodeFlag::HideCaption) return;

    QFont f = painter.font();
    bool isBold = f.bold();
    f.setBold(true);

    QRectF rect = m_geometry->captionRect();

    painter.setFont(f);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(gt::gui::color::text());
    painter.drawText(rect, node.caption(), QTextOption{Qt::AlignHCenter});

    if (false)
    {
        painter.setPen(Qt::white);
        painter.drawRect(m_geometry->captionRect());
    }

    f.setBold(isBold);
    painter.setFont(f);
}

void
NodePainter::paint(QPainter& painter)
{
    drawRect(painter);
    drawPorts(painter);
    drawCaption(painter);
}
