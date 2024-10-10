/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H
#define GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H

#include <intelli/globals.h>

#include <QPointer>
#include <QTimeLine>
#include <QGraphicsObject>

namespace intelli
{

class Node;
class NodePainter;

/**
 * @brief Graphic object used to visualize the eval state of the node in the
 * graph. Also applies a tooltip for the current eval state.
 */
class NodeEvalStateGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::NodeEvalState };
    int type() const override { return Type; }

    NodeEvalStateGraphicsObject(QGraphicsObject& parent,
                                NodePainter& painter,
                                Node& node);

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

private:

    /// Associated node
    QPointer<Node> m_node;
    /// Timeline for anmation
    QTimeLine m_timeLine;
    /// Painter used for rendering
    NodePainter* m_painter = nullptr;
    /// Node eval state that is displayed currently
    NodeEvalState m_state = NodeEvalState::Invalid;

    void paintRunningState(QPainter& painter);

    void paintPausedState(QPainter& painter);

    void paintIdleState(QPainter& painter);

private slots:

    /**
     * @brief Updates the node eval state, used for the visualization of the
     * eval state for the user.
     */
    void onNodeEvalStateChanged();
};

} // namespace intelli

#endif // GT_INTELLI_NODEEVALSTATEGRAPHICSOBJECT_H
