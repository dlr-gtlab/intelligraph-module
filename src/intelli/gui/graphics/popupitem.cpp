/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/popupitem.h>
#include <intelli/gui/style.h>

#include <gt_colors.h>

#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <QPointer>
#include <QTimer>

using namespace intelli;

static PopupItem* s_activeItem{};

PopupItem::PopupItem(QGraphicsScene& scene, QString const& text, seconds timeout)
{
    setFlag(GraphicsItemFlag::ItemContainsChildrenInShape, true);
    // not affected by zoom level
    setFlag(GraphicsItemFlag::ItemIgnoresTransformations, true);
    setFlag(GraphicsItemFlag::ItemIsSelectable, false);
    setFlag(GraphicsItemFlag::ItemIsMovable, false);

    auto* textItem = new QGraphicsTextItem(text, this);
    textItem->setDefaultTextColor(gt::gui::color::text());

    auto* backgroundItem = new QGraphicsRectItem(textItem->boundingRect(), this);
    backgroundItem->setPen(QPen{Qt::gray});
    backgroundItem->setBrush(QBrush{style::invert(gt::gui::color::text())});

    backgroundItem->setZValue(1);
    textItem->setZValue(2);
    setZValue(style::zValue(style::ZValue::Popup));

    scene.addItem(this);

    if (s_activeItem) s_activeItem->hide();
    s_activeItem = this;

    // setup fading animation
    m_timeLine.setEasingCurve(QEasingCurve::Linear);
    m_timeLine.setLoopCount(1);
    m_timeLine.setDuration(std::chrono::milliseconds{700}.count());

    int fps = 1000 / m_timeLine.updateInterval();
    m_timeLine.setFrameRange(0, fps);

    QObject::connect(&m_timeLine, &QTimeLine::frameChanged, &scene, [this](int value){
        setOpacity(1 - m_timeLine.currentValue());
        update();
    });

    QObject::connect(&m_timeLine, &QTimeLine::finished, &scene, [this](){
        delete this;
    });

    QTimer::singleShot(timeout, &scene, [this](){
        m_timeLine.start();
    });
}

PopupItem::~PopupItem()
{
    if (s_activeItem == this) s_activeItem = nullptr;
}

QRectF
PopupItem::boundingRect() const
{
    return childrenBoundingRect();
}

void
PopupItem::paint(QPainter* painter,
                 QStyleOptionGraphicsItem const* option,
                 QWidget* widget)
{
    for (QGraphicsItem* child : childItems())
    {
        child->paint(painter, option, widget);
    }
}

