/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEGRAPHICSOBJECT_H

#include <intelli/nodedatainterface.h>
#include <intelli/node.h>
#include <intelli/gui/nodeui.h>

#include <QPointer>
#include <QGraphicsObject>

namespace intelli
{

class NodeEvalStateGraphicsObject;

class GT_INTELLI_EXPORT NodeGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    NodeGraphicsObject(Graph& graph, Node& node, NodeUI& ui);

    Node* node();
    Node const* node() const;

    bool isHovered() const;

    QRectF boundingRect() const override;

    void setNodeEvalState(NodeEvalState state);

    void repositionEvalStateVisualizer(QPointF pos);

    void embedCentralWidget();

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

signals:

    void nodeClicked(Node* node);

    void portContextMenuRequested(Node* node, PortId port, QPointF pos);

    void contextMenuRequested(Node* node, QPointF pos);

private:

    enum State
    {
        Normal = 0,
        Resizing,
        Translating
    };

    QPointer<Graph> m_graph;
    QPointer<Node> m_node;
    QPointer<NodeUI> m_ui;
    QPointer<QGraphicsProxyWidget> m_proxyWidget;
    NodeEvalStateGraphicsObject* m_evalStateObject;
    State m_state = Normal;
    bool m_hovered = false;
};

} // namespace intelli

#endif // GT_INTELLI_NODEGRAPHICSOBJECT_H
