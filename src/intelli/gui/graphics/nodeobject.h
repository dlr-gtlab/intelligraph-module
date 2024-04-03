/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEGRAPHICSOBJECT_H

#include <intelli/node.h>
#include <intelli/nodedatainterface.h>
#include <intelli/gui/nodegeometry.h>
#include <intelli/gui/nodepainter.h>

#include <QPointer>
#include <QGraphicsObject>
#include <QGraphicsProxyWidget>

namespace intelli
{

class NodeUI;
class NodeEvalStateGraphicsObject;

class GT_INTELLI_EXPORT NodeGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::Node };
    int type() const override { return Type; }

    NodeGraphicsObject(Graph& graph, Node& node, NodeUI& ui);

    Node& node();
    Node const& node() const;

    NodeId nodeId() const;

    Graph& graph();
    Graph const& graph() const;

    bool isHovered() const;

    QRectF boundingRect() const override;

    QPainterPath shape() const override;

    QGraphicsWidget* centralWidget();
    QGraphicsWidget const* centralWidget() const;

    NodeGeometry const& geometry() const;

    void moveConnections();

    void embedCentralWidget();

    void commitPosition();

    void setNodeEvalState(NodeEvalState state);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

public slots:

    void onNodeChanged();

signals:

    void makeDraftConnection(NodeGraphicsObject* object, ConnectionId conId);

    void makeDraftConnection(NodeGraphicsObject* object, PortType type, PortId port);

    void nodeShifted(NodeGraphicsObject* object, QPointF diff);

    void nodeMoved(NodeGraphicsObject* object);

    void nodeGeometryChanged(NodeGraphicsObject* object);

    void portContextMenuRequested(NodeGraphicsObject* object, PortId port, QPointF pos);

    void contextMenuRequested(NodeGraphicsObject* object, QPointF pos);

private:

    struct Impl;

    enum State
    {
        Normal = 0,
        Translating,
        Resizing
    };

    QPointer<Graph> m_graph;
    QPointer<Node> m_node;
    std::unique_ptr<NodeGeometry> m_geometry;
    std::unique_ptr<NodePainter> m_painter;
    QPointer<QGraphicsProxyWidget> m_proxyWidget = nullptr;
    NodeEvalStateGraphicsObject* m_evalStateObject = nullptr;

    // flags
    State m_state = Normal;
    bool m_hovered = false;

private slots:

    void updateChildItems();
};

} // namespace intelli

#endif // GT_INTELLI_NODEGRAPHICSOBJECT_H
