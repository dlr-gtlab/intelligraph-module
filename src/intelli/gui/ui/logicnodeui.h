/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 2.4.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef LOGICNODEUI_H
#define LOGICNODEUI_H

#include <intelli/gui/nodeui.h>
#include <intelli/gui/nodepainter.h>
#include <intelli/gui/nodegeometry.h>

namespace intelli
{

class LogicNode;
class LogicNodeGeometry : public NodeGeometry
{
public:

    LogicNodeGeometry(Node& node);

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

class LogicNodePainter : public NodePainter
{
public:

    LogicNodePainter(NodeGraphicsObject& object, NodeGeometry& geometry);

    void drawBackground(QPainter& painter) const override;

    void drawOutline(QPainter& painter) const override;

    void drawPortCaption(QPainter& painter,
                         PortData& port,
                         PortType type,
                         PortIndex idx,
                         bool connected) const override;
};



class LogicNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE LogicNodeUI();

    virtual std::unique_ptr<NodePainter> painter(NodeGraphicsObject& object, NodeGeometry& geometry) const;

    virtual std::unique_ptr<NodeGeometry> geometry(Node& node) const;
};

} // namespace intelli

#endif // LOGICNODEUI_H
