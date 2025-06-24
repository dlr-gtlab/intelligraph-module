/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/graphics/graphicsobject.h>

#include <QGraphicsSceneHoverEvent>

using namespace intelli;

GraphicsObject::GraphicsObject(QGraphicsItem* parent) :
    QGraphicsObject(parent)
{

}

void
GraphicsObject::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    m_hovered = true;
    emit hoveredChanged(QPrivateSignal());
    update();
}

void
GraphicsObject::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();

    m_hovered = false;
    emit hoveredChanged(QPrivateSignal());
    update();
}

