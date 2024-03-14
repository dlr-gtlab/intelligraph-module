/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H

#include <intelli/node.h>
#include <intelli/gui/nodeui.h>
#include <intelli/nodedatainterface.h>

#include <QPointer>
#include <QTimeLine>
#include <QGraphicsObject>

namespace intelli
{

class NodeEvalStateGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    NodeEvalStateGraphicsObject(QGraphicsObject& parent, Node& node, NodeUI& ui);

    QRectF boundingRect() const override;

    void setNodeEvalState(NodeEvalState state);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

private:

    QPointer<Node> m_node;
    QPointer<NodeUI> m_ui;
    QTimeLine m_timeLine;
    NodeEvalState m_state = NodeEvalState::Invalid;

    void paintRunningState(QPainter& painter);

    void paintPausedState(QPainter& painter);

    void paintIdleState(QPainter& painter);
};

} // namespace intelli

#endif // GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H
