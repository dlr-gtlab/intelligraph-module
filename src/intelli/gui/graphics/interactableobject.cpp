/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/interactableobject.h>
#include <intelli/gui/style.h>
#include <intelli/utilities.h>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsScene>
#include <QCursor>

#include <cmath>

#include <QPainter>

using namespace intelli;

class DropShadowObject : public QGraphicsObject
{
public:

    using GetBoundingRectFunctor = std::function<QRectF()>;
    using PaintDropShadowFunctor = std::function<void(QPainter&)>;

    DropShadowObject(QGraphicsScene* scene,
                     InteractableGraphicsObject& parent,
                     std::function<QRectF()> boundingRectFunctor,
                     std::function<void(QPainter&)> paintFunctor) :
        m_boundingRect(boundingRectFunctor),
        m_paint(paintFunctor)
    {
        setFlag(QGraphicsItem::ItemIsFocusable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemIsSelectable, false);

        setZValue(style::zValue(style::ZValue::Background));

        if (scene) scene->addItem(this);

        auto updateDropShadowPos = [this, p = &parent](){
            setPos(p->pos() + style::currentStyle().node.dropShadowOffset);
        };
        auto updateDropShadowGeometry = [this](){
            prepareGeometryChange();
            update();
        };

        connect(&parent, &InteractableGraphicsObject::objectMoved,
                this, updateDropShadowPos);
        connect(&parent, &InteractableGraphicsObject::objectShifted,
                this, updateDropShadowPos);
        connect(&parent, &QGraphicsObject::xChanged,
                this, updateDropShadowPos);
        connect(&parent, &QGraphicsObject::yChanged,
                this, updateDropShadowPos);
        connect(&parent, &InteractableGraphicsObject::objectResized,
                this, updateDropShadowGeometry);
        connect(&parent, &QObject::destroyed, this, [this](){
            delete this;
        });

        updateDropShadowPos();
    }

    QRectF boundingRect() const
    {
        assert(m_paint);
        return m_boundingRect();
    }

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const*,
               QWidget*)
    {
        assert(m_paint);
        m_paint(*painter);
    }

private:

    GetBoundingRectFunctor m_boundingRect;
    PaintDropShadowFunctor m_paint;
};

////////////////////////////////////////////////////////////////////////////////

InteractableGraphicsObject::InteractableGraphicsObject(GraphSceneData const& data,
                                                       QGraphicsItem* parent) :
    GraphicsObject(parent),
    m_sceneData(&data)
{

}

InteractableGraphicsObject::~InteractableGraphicsObject() = default;

void
InteractableGraphicsObject::setInteractionFlag(InteractionFlag flag, bool enable)
{
    enable ? m_flags |= flag : m_flags &= ~flag;
}

QGraphicsObject*
InteractableGraphicsObject::setupDropShadowEffect(std::function<QRectF()> boundingRectFunctor,
                                                  std::function<void (QPainter&)> paintFunctor)
{
    return new DropShadowObject{
        scene(), *this, boundingRectFunctor, paintFunctor
    };
}

void
InteractableGraphicsObject::shiftBy(double x, double y)
{
    if (interactionFlags() & AllowTranslation)
    {
        moveBy(x, y);
    }
}

void
InteractableGraphicsObject::alignToGrid()
{
    if (m_sceneData->gridSize <= 0) return;

    QPoint newPos = utils::quantize(pos(), m_sceneData->gridSize);
    setPos(newPos);
    commitPosition();
}

GraphSceneData const&
InteractableGraphicsObject::sceneData() const
{
    return *m_sceneData;
}

bool
InteractableGraphicsObject::isCollapsed() const
{
    return m_collapsed;
}

void
InteractableGraphicsObject::collapse(bool doCollapse)
{
    if (isCollapsed() == doCollapse) return; // nothing to do

    prepareGeometryChange();

    m_collapsed = doCollapse;

    emit objectCollapsed(this, doCollapse);
    emit objectResized(this);
}

void
InteractableGraphicsObject::setCollapsed(bool doCollapse)
{
    collapse(doCollapse);
}

QRectF
InteractableGraphicsObject::widgetSceneBoundingRect() const
{
    // nothing to do
    return {};
}

void
InteractableGraphicsObject::commitPosition()
{
    // nothing to do
}

void
InteractableGraphicsObject::setupContextMenu(QMenu& menu)
{
    Q_UNUSED(menu);
    // nothing to do
}

void
InteractableGraphicsObject::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        return event->ignore();
    }

    event->accept();

    // handle resizing
    if (canResize(event->pos()))
    {
        if (!(interactionFlags() & AllowResizing)) return;

        m_state = State::Resizing;
        m_translationStart = {0.0, 0.0};
        return;
    }

    if (interactionFlags() & AllowTranslation)
    {
        // handle translating
        m_state = State::Translating;
        m_translationStart = pos();
    }

    // update selection
    if (!(event->modifiers() & Qt::ControlModifier))
    {
        if (isSelected()) return;

        QGraphicsScene* scene = this->scene();
        assert(scene);
        scene->clearSelection();

        setSelected(true);
        return;
    }

    setSelected(!isSelected());

    if (!isSelected()) m_state = State::Normal;
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
        m_translationStart.rx() = diff.x() - std::floor(diff.x());
        m_translationStart.ry() = diff.y() - std::floor(diff.y());

        resizeBy(QSize{(int)floor(diff.x()), (int)floor(diff.y())});

        emit objectResized(this);
        break;

    case State::Translating:
        m_translationStart += diff;

        if (m_sceneData->gridSize > 0 && (
             (!m_sceneData->snapToGrid &&   event->modifiers() & Qt::ControlModifier) ||
             ( m_sceneData->snapToGrid && !(event->modifiers() & Qt::AltModifier   ))
            ))
        {
            QPoint newPos = utils::quantize(m_translationStart, m_sceneData->gridSize);

            // position not changed
            if (newPos == pos()) return event->accept();

            diff = newPos - pos();
        }

        assert(scene());
        for (QGraphicsItem* item : scene()->selectedItems())
        {
            if (auto* object = graphics_cast<InteractableGraphicsObject*>(item))
            {
                object->shiftBy(diff.x(), diff.y());
            }
        }

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
        assert(scene());
        for (QGraphicsItem* item : scene()->selectedItems())
        {
            if (auto* object = graphics_cast<InteractableGraphicsObject*>(item))
            {
                object->commitPosition();
            }
        }
        emit objectMoved(this);
        break;
    }

    m_state = State::Normal;

    event->accept();
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
InteractableGraphicsObject::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    event->accept();
    emit contextMenuRequested(this);
}
