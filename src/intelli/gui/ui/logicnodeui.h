/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_LOGICNODEUI_H
#define GT_INTELLI_LOGICNODEUI_H

#include <intelli/gui/nodeui.h>
#include <intelli/gui/nodepainter.h>
#include <intelli/gui/nodegeometry.h>

namespace intelli
{

class LogicNode;
/**
 * @brief The LogicNodeUI class.
 * Geometry class for the `LogicNode`. Describes the shape of the Gates shapes
 * of the boolean opeations.
 */
class LogicNodeGeometry : public NodeGeometry
{
public:

    LogicNodeGeometry(Node const& node);

    QRectF captionRect() const override;

    QRectF evalStateRect() const override;

    QRectF portRect(PortType type, PortIndex idx) const override;

    QPainterPath beginCurve() const;

    void applyLeftCurve(QPainterPath& path) const;

    void applyRightCurve(QPainterPath& path) const;

    LogicNode const& logicNode() const;

protected:

    QPainterPath computeShape() const override;

    QRectF computeInnerRect() const override;

    QRectF computeBoundingRect() const override;
};

/**
 * @brief The LogicNodeUI class.
 * Painter class for the `LogicNode`. Renders the Gate shapes of the boolean
 * opeations.
 */
class LogicNodePainter : public NodePainter
{
public:

    LogicNodePainter(NodeGraphicsObject const& object,
                     NodeGeometry const& geometry);

    void drawBackground(QPainter& painter) const override;

    void drawOutline(QPainter& painter) const override;

    void drawPortCaption(QPainter& painter,
                         PortInfo const& port,
                         PortType type,
                         PortIndex idx,
                         uint flags) const override;
};

/**
 * @brief The LogicNodeUI class.
 * UI class for the `LogicNode`
 */
class LogicNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE LogicNodeUI();

    virtual std::unique_ptr<NodePainter> painter(NodeGraphicsObject const& object,
                                                 NodeGeometry const& geometry) const;

    virtual std::unique_ptr<NodeGeometry> geometry(Node const& node) const;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICNODEUI_H
