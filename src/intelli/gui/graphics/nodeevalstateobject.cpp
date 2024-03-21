/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 12.3.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include <intelli/gui/graphics/nodeevalstateobject.h>
#include <intelli/gui/style.h>

#include <gt_colors.h>
#include <gt_icons.h>

#include <array>

#include <QtCore/QtMath>
#include <QtGui/QPainter>

using namespace intelli;

NodeEvalStateGraphicsObject::NodeEvalStateGraphicsObject(QGraphicsObject& parent,
                                                         Node& node,
                                                         NodePainter& painter) :
    QGraphicsObject(&parent),
    m_node(&node),
    m_timeLine(1000),
    m_painter(&painter)
{
    setZValue(10);
    m_timeLine.setEasingCurve(QEasingCurve::Linear);
    m_timeLine.setLoopCount(0);
    m_timeLine.setFrameRange(0, 24);

    connect(&m_timeLine, &QTimeLine::frameChanged, this, [=](){
        this->update();
    });
}

QRectF
NodeEvalStateGraphicsObject::boundingRect() const
{
    return QRectF(QPoint{0, 0}, QSize{20, 20});
}

void
NodeEvalStateGraphicsObject::setNodeEvalState(NodeEvalState state)
{
    m_state = state;

    if (m_state == NodeEvalState::Evaluating)
    {
        if (m_timeLine.state() != QTimeLine::Running) m_timeLine.start();
    }
    else
    {
        m_timeLine.stop();
    }

    update();
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
NodeEvalStateGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    return QGraphicsObject::mousePressEvent(event);
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
    }

    QColor const& backgroundColor = m_painter->backgroundColor();
    bool lighter = backgroundColor.lightnessF() <= 0.5;
    *color = color->lighter(100 + (lighter ? 50 : 0));

    QBrush brush(*color, Qt::SolidPattern);
    QPen pen(brush, 1, Qt::SolidLine);

    painter.setPen(pen);
    painter.setBrush(brush);

    painter.drawEllipse(center.x() - size/2, center.y() - size/2, size, size);
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
    bool lighter = backgroundColor.lightnessF() <= 0.5;

    // color gradient
    constexpr int colorIncrement = 100 / N;

    int offset = colorIncrement;
    for (QColor& color : colors)
    {
        color = gt::gui::color::lighten(backgroundColor, lighter ? -offset : offset);
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
