/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H
#define GT_INTELLI_CONNECTIONGRAPHICSOBJECT_H

#include <intelli/memory.h>
#include <intelli/gui/connectiongeometry.h>
#include <intelli/gui/graphics/graphicsobject.h>

namespace intelli
{

class Graph;
class NodeGraphicsObject;

/**
 * @brief Graphics object used to represent a connection between to an output
 * and input port of two different nodes. Does not update the connection
 * automatically, this must be triggered by the graphics scene. It is possible
 * to apply different shapes to the connection.
 * The `pos` of this object is not representative of its actual position.
 */
class GT_INTELLI_TEST_EXPORT ConnectionGraphicsObject : public GraphicsObject
{
    Q_OBJECT

public:

    // Needed for graphics_cast
    enum { Type = make_graphics_type<GraphicsItemType::Connection, GraphicsObject>() };
    int type() const override { return Type; }

    static std::unique_ptr<ConnectionGraphicsObject>
    makeConnection(QGraphicsScene& scene,
                   Graph& graph,
                   ConnectionId conId,
                   NodeGraphicsObject const& outNodeObj,
                   NodeGraphicsObject const& inNodeObj);

    static std::unique_ptr<ConnectionGraphicsObject>
    makeDraftConnection(QGraphicsScene& scene,
                        ConnectionId draftConId,
                        NodeGraphicsObject const& startObj);

    ~ConnectionGraphicsObject();

    DeleteOrdering deleteOrdering() const override { return DeleteFirst; }

    bool deleteObject() override;

    /**
     * @brief Whether this connection is a draft.
     * @return Is a draft connection.
     */
    bool isDraft() const;

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

    /**
     * @brief The connection id this object refers to
     * @return Connection id
     */
    ConnectionId connectionId() const;

    /**
     * @brief Returns the corresponding end point of the connection
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @return End/start point respectively
     */
    QPointF endPoint(PortType type) const;

    /**
     * @brief Setter for the corresponding end point of this connection.
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @param pos New position of the end/start point
     */
    void setEndPoint(PortType type, QPointF pos);

    /**
     * @brief Sets the type id of the end/start point
     * @param type Denotes the end point for `PortType::In` or the
     * start point for `PortType::Out`.
     * @param typeId New type id for the end/start point
     */
    void setPortTypeId(PortType type, TypeId typeId);

    /**
     * @brief Setter for the connection shape
     * @param shape
     */
    void setConnectionShape(ConnectionShape shape);

    /**
     * @brief Deemphasizes this object, i.e. to visually highlight other
     * objects.
     * @param inactive Whether this object should be made inactive
     */
    void makeInactive(bool inactive = true);

signals:

    /**
     * @brief Emitted once a draft connection should be finalized, which
     * either means creating a new connection or aborting the
     * creation of a draft connection if `conId` is invalid
     * @param conId Final connection id. May be invalid, indicating that the
     * creation should be aborted.
     */
    void finalizeDraftConnnection(ConnectionId conId);

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

    QVariant itemChange(GraphicsItemChange change, QVariant const& value) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:

    /// Pointer to graph object
    QPointer<Graph> m_graph;
    /// pointers to outNode and inNode i.e. the start and end node
    QPointer<NodeGraphicsObject const> m_outNode, m_inNode;
    /// Connection id
    ConnectionId m_connection;
    /// Type ids for the start and end point
    TypeId m_outType, m_inType;
    /// Geometry info of current connection
    ConnectionGeometry m_geometry;
    /// The shape of the connection
    ConnectionShape m_shape = ConnectionShape::DefaultShape;
    /// Start and end point of the connection
    QPointF m_start, m_end;
    /// Whether this object is considered inactive
    bool m_inactive = false;

    /**
     * @brief constructor. In case the connection is a draft connection, either
     * `outNodeObj` or `inNodeObj` must be not null, else both objects must
     * not be null.
     * @param scene Scene this object will be added to
     * @param graph Graph object. May be invalid if the connection is a draft.
     * @param connection ConnectionId to render. May be partially invalid,
     * indicating a draft connection.
     * @param outNodeObj graphics object for the output side
     * @param inNodeObj graphics object for the output side
     */
    explicit ConnectionGraphicsObject(QGraphicsScene& scene,
                                      Graph* graph,
                                      ConnectionId connection,
                                      NodeGraphicsObject const* outNodeObj = {},
                                      NodeGraphicsObject const* inNodeObj = {});

    QPointF calcEndPoint(NodeGraphicsObject const* nodeObj,
                         PortType portType,
                         PortId portId);
};

} // namespace intelli

#endif // CONNECTIONGRAPHICSOBJECT_H
