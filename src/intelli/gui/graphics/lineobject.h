/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LINEGRAPHICSOBJECT_H
#define GT_INTELLI_LINEGRAPHICSOBJECT_H

#include <intelli/globals.h>
#include <intelli/gui/graphics/graphicsobject.h>
#include <intelli/gui/graphics/interactableobject.h>
#include <intelli/gui/connectiongeometry.h>

#include <QPointer>

namespace intelli
{

// TODO: description
class GT_INTELLI_TEST_EXPORT LineGraphicsObject : public GraphicsObject
{
    Q_OBJECT

public:

    // Needed for graphics_cast
    enum { Type = make_graphics_type<GraphicsItemType::Line, GraphicsObject>() };
    int type() const override { return Type; }

    static std::unique_ptr<LineGraphicsObject>
    makeLine(InteractableGraphicsObject const& startObj,
             InteractableGraphicsObject const& endObj);

    static std::unique_ptr<LineGraphicsObject>
    makeDraftLine(InteractableGraphicsObject const& startObj);

    ~LineGraphicsObject();

    DeleteOrdering deleteOrdering() const override { return DeleteFirst; }

    bool deleteObject() override;

    bool isDraft() const;

    void setTypeMask(size_t mask);

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

    /**
     * @brief Shape used for collision detection
     * @return Shape
     */
    QPainterPath shape() const override;

    GraphicsObject const* startItem() const;

    GraphicsObject const* endItem() const;

    void setEndPoint(PortType type, QGraphicsItem const& object);

    void setEndPoint(PortType type, QPointF pos);

public slots:

    void updateEndPoints();

signals:

    void deleteRequested();

    void finalizeDraftConnection(QGraphicsItem* endItem);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:

    /// start and end item
    QPointer<InteractableGraphicsObject const> m_startItem, m_endItem;
    /// geometry object
    ConnectionGeometry m_geometry;
    /// Start and end point
    QPointF m_start, m_end;
    // TODO: description
    size_t m_mask{};

    LineGraphicsObject(InteractableGraphicsObject const& start,
                       InteractableGraphicsObject const* end = nullptr);

    void updateEndPoint();
};

} // namespace intelli

#endif // GT_INTELLI_LINEGRAPHICSOBJECT_H
