/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/interactableobject.h>

#include <intelli/gui/style.h>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>
#include <QCursor>

using namespace intelli;

InteractableGraphicsObject::InteractableGraphicsObject(GraphSceneData const& data,
                                                       QGraphicsObject* parent) :
    QGraphicsObject(parent),
    m_sceneData(&data)
{
    setZValue(style::zValue(style::ZValue::Node));
}

InteractableGraphicsObject::~InteractableGraphicsObject() = default;

void
InteractableGraphicsObject::collapse(bool doCollapse)
{
    if (isCollapsed() == doCollapse) return; // nothing to do

    prepareGeometryChange();

    m_collapsed = doCollapse;

    emit objectCollapsed(this, doCollapse);
}

void
InteractableGraphicsObject::setCollapse(bool doCollapse)
{
    return collapse(doCollapse);
}

void
InteractableGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        return event->ignore();
    }

    event->accept();

    // bring this node forward
    setZValue(style::zValue(style::ZValue::NodeHovered));

    // handle resizing
    if (canResize(event->pos()))
    {
        m_state = State::Resizing;
        m_translationStart = {0.0, 0.0};
        return;
    }

    // handle translating
    m_state = State::Translating;
    m_translationStart = pos();

    // update selection
    if (!isSelected())
    {
        if (!(event->modifiers() & Qt::ControlModifier))
        {
            QGraphicsScene* scene = this->scene();
            assert(scene);
            scene->clearSelection();
        }

        setSelected(true);
    }
}

void
InteractableGraphicsObject::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPointF diff = event->pos() - event->lastPos();

    switch (m_state)
    {
    case State::Resizing:
        // round down to integers but keep remainder
        diff += m_translationStart;
        m_translationStart.rx() = diff.x() - floor(diff.x());
        m_translationStart.ry() = diff.y() - floor(diff.y());

        resize(QSize{(int)floor(diff.x()), (int)floor(diff.y())});
        break;

    case State::Translating:
        m_translationStart += diff;

        if ((m_sceneData->snapToGrid || event->modifiers() & Qt::ControlModifier)
            && m_sceneData->gridSize > 0)
        {
            QPoint newPos = quantize(m_translationStart, m_sceneData->gridSize);

            // position not changed
            if (newPos == pos()) return event->accept();

            diff = newPos - pos();
        }

        moveBy(diff.x(), diff.y());
        emit objectShifted(this, diff);
        break;

    case State::Normal:
    default:
        return event->ignore();
    }

    event->accept();
}

void
InteractableGraphicsObject::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    switch (m_state)
    {
    default:
    case State::Normal:
        return event->ignore();

    case State::Resizing:
        break;

    case State::Translating:
        emit objectMoved(this);
        break;
    }

    m_state = State::Normal;

    event->accept();
}

void
InteractableGraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    setZValue(style::zValue(style::ZValue::NodeHovered));

    m_hovered = true;
    update();
}

void
InteractableGraphicsObject::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    QPointF pos = event->pos();

    // check for resize handle hit and change cursor
    if (canResize(pos))
    {
        setCursor(QCursor(Qt::SizeFDiagCursor));
        return;
    }

    setCursor(QCursor());
}

void
InteractableGraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    if (!isSelected())
    {
        setZValue(style::zValue(style::ZValue::Node));
    }

    m_hovered = false;
    update();
}
