/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/nodeevalstateobject.h>
#include <intelli/node.h>
#include <intelli/gui/nodepainter.h>
#include <intelli/gui/style.h>

#include <gt_icons.h>

#include <array>

#include <QtCore/QtMath>
#include <QtGui/QPainter>

using namespace intelli;

NodeEvalStateGraphicsObject::NodeEvalStateGraphicsObject(QGraphicsObject& parent,
                                                         NodePainter& painter,
                                                         Node& node) :
    QGraphicsObject(&parent),
    m_node(&node),
    m_timeLine(1000),
    m_painter(&painter)
{
    setZValue(style::zValue(style::ZValue::NodeEvalState));

    m_timeLine.setEasingCurve(QEasingCurve::Linear);
    m_timeLine.setLoopCount(0);
    m_timeLine.setFrameRange(0, 24);

    connect(&m_timeLine, &QTimeLine::frameChanged,
            m_node, [=](){ this->update(); });

    connect(m_node, &Node::nodeEvalStateChanged,
            this, &NodeEvalStateGraphicsObject::onNodeEvalStateChanged,
            Qt::DirectConnection);

    // update once
    onNodeEvalStateChanged();
}

QRectF
NodeEvalStateGraphicsObject::boundingRect() const
{
    auto& style = style::currentStyle();
    return QRectF{QPoint{0, 0},
                  QSizeF{style.node.evalStateSize, style.node.evalStateSize}};
}

void
NodeEvalStateGraphicsObject::paint(QPainter* painter,
                                   QStyleOptionGraphicsItem const* option,
                                   QWidget* widget)
{
    assert(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // draw paused state
    switch (m_state)
    {
    case NodeEvalState::Paused:
        return paintPausedState(*painter);
    case NodeEvalState::Evaluating:
        return paintRunningState(*painter);
    case NodeEvalState::Invalid:
    case NodeEvalState::Outdated:
    case NodeEvalState::Valid:
        return paintIdleState(*painter);
    }
}

void
NodeEvalStateGraphicsObject::paintIdleState(QPainter& painter)
{
    QRectF rect = boundingRect();
    QPointF center = rect.center();

    constexpr double sizePercentage = 0.4;
    int size = sizePercentage * rect.height();
    size += (size & 1); // round up to even

    QColor colorsInvalid  = Qt::red;
    QColor colorsOutdated = Qt::yellow;
    QColor colorsValid    = Qt::green;

    QColor* color = &colorsInvalid;

    switch (m_state)
    {
    case NodeEvalState::Invalid:
        color = &colorsInvalid;
        break;
    case NodeEvalState::Valid:
        color = &colorsValid;
        break;
    case NodeEvalState::Outdated:
        color = &colorsOutdated;
        break;
    case NodeEvalState::Evaluating:
    case NodeEvalState::Paused:
        assert(!"code is unreachable");
        break;
    }

    QColor const& backgroundColor = m_painter->backgroundColor();
    bool isBright = backgroundColor.lightness() > 125;
    *color = style::tint(*color, isBright ? -15 : 50);

    QBrush brush(*color, Qt::SolidPattern);
    QPen pen(brush, 1, Qt::SolidLine);

    painter.setPen(pen);
    painter.setBrush(brush);

    painter.drawEllipse(center.x() - size/2, center.y() - size/2, size, size);
}

void
NodeEvalStateGraphicsObject::onNodeEvalStateChanged()
{
    assert(m_node);
    m_state = m_node->nodeEvalState();

    m_timeLine.stop();

    switch (m_state)
    {
    case NodeEvalState::Invalid:
        setToolTip(tr("Node failed to evaluate or is not setup correctly"));
        break;
    case NodeEvalState::Outdated:
        setToolTip(tr("Node data is outdated and must be reevaluated"));
        break;
    case NodeEvalState::Paused:
        setToolTip(tr("Node is paused and will not be evaluated automatically"));
        break;
    case NodeEvalState::Valid:
        setToolTip(tr("Node has been evaluated successfully"));
        break;
    case NodeEvalState::Evaluating:
        setToolTip(tr("Node is currently evaluating"));
        m_timeLine.start();
        break;
    }

    update();
}

void
NodeEvalStateGraphicsObject::paintRunningState(QPainter& painter)
{
    // number of circles
    constexpr size_t N = 5;

    QRectF rect = boundingRect();
    QPointF center = rect.center();

    constexpr double sizePercentage = 0.7;

    // radius
    constexpr qreal circleDiameter = 4;
    constexpr qreal radius = circleDiameter / 2.0;

    // store positions of the circles
    std::array<QPointF, N> circlePositions;

    // divide the circle into equal parts
    constexpr qreal FULL_RADIUS = 360.0;
    constexpr qreal angleIncrement = FULL_RADIUS / N;

    qreal angle = 0.0;
    angle += FULL_RADIUS * m_timeLine.currentValue();
    for (QPointF& pos : circlePositions)
    {
        qreal x = center.x() + ((rect.width() - radius) * 0.5 * qCos(qDegreesToRadians(angle)) * sizePercentage);
        qreal y = center.y() - ((rect.width() - radius) * 0.5 * qSin(qDegreesToRadians(angle)) * sizePercentage);
        pos = QPointF(x, y);
        angle += angleIncrement;
    }

    // store colors for each circle
    std::array<QColor, N> colors;

    QColor const& backgroundColor = m_painter->backgroundColor();
    bool isBright = backgroundColor.lightness() > 125;

    // color gradient
    constexpr int colorIncrement = 100 / N;

    int offset = colorIncrement;
    for (QColor& color : colors)
    {
        color = style::tint(backgroundColor, isBright ? -offset : offset);
        offset += colorIncrement;
    }

    // Set the pen and brush for the circles
    QBrush brush(Qt::SolidPattern);
    QPen pen(brush, 1, Qt::SolidLine);

    pen.setColor(Qt::white);
    brush.setColor(Qt::white);
    painter.setPen(pen);
    painter.setBrush(brush);

    // Draw the 5 circles
    for (size_t i = 0; i < N; ++i)
    {
        // set color
        pen.setColor(colors[i]);
        brush.setColor(colors[i]);
        painter.setPen(pen);
        painter.setBrush(brush);
        // set position
        painter.drawEllipse(circlePositions[i], radius, radius);
    }
}

void
NodeEvalStateGraphicsObject::paintPausedState(QPainter& painter)
{
    QRectF rect = boundingRect();

    gt::gui::icon::pause().paint(&painter, rect.toRect());
}
