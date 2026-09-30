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
#include <QWidget>

class QLCDNumber;

namespace intelli
{

class LogicNode;
class BinaryDisplayNode;
class NodeGraphicsObject;
/**
 * @brief The LogicNodeUI class.
 * Geometry class for the `LogicNode`. Describes the shape of the Gates shapes
 * of the boolean opeations.
 */
class LogicNodeGeometry : public NodeGeometry
{
public:

    LogicNodeGeometry(NodeGraphicsObject const& object);

    QRectF portRect(PortType type, PortIndex idx) const override;

    QPainterPath beginCurve() const;

    void applyLeftCurve(QPainterPath& path) const;

    void applyRightCurve(QPainterPath& path) const;

    LogicNode const& logicNode() const;

protected:

    QPainterPath computeShape() const override;

    QRectF computeNodeBodyRect() const override;

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

    void drawBackground(QPainter& painter, uint flags = DefaultNodeRenderFlags) const override;

    void drawPortCaption(QPainter& painter,
                         PortInfo const& port,
                         PortType type,
                         PortIndex idx,
                         uint flags) const override;

    void drawPort(QPainter& painter,
                  PortInfo const& port,
                  PortType type,
                  PortIndex idx,
                  uint flags) const override;
};

class BinaryDisplayNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit BinaryDisplayNodeWidget(BinaryDisplayNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateDisplay();
    void updateDigitCount();
    void updateStyle(bool isDark);

private:

    BinaryDisplayNode* m_node{};
    QLCDNumber* m_display{};
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

    std::unique_ptr<NodePainter> painter(NodeGraphicsObject const& object,
                                         NodeGeometry const& geometry) const override;

    std::unique_ptr<NodeGeometry> geometry(NodeGraphicsObject const& object) const override;

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_LOGICNODEUI_H
