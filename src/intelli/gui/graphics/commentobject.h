/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGRAPHICSOBJECT_H
#define GT_INTELLI_COMMENTGRAPHICSOBJECT_H

#include <intelli/globals.h>

#include <QGraphicsObject>

class QGraphicsProxyWidget;
class QTextEdit;

namespace intelli
{

class CommentGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    // Needed for qgraphicsitem_cast
    enum { Type = UserType + (int)GraphicsItemType::N_ENTRIES + 1 };
    int type() const override { return Type; }

    CommentGraphicsObject();

    QRectF boundingRect() const override;

protected:

    QRectF resizeHandleRect() const;

    void paint(QPainter *painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:

    class ProxyWidget;
    class TextEdit;

    enum State
    {
        Normal = 0,
        Translating,
        Resizing,
        Editing,
    };

    State state = Normal;
    QPointF translationDiff;

    ProxyWidget* proxyWidget;
    QTextEdit* editor;

    bool hovered = false;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGRAPHICSOBJECT_H
