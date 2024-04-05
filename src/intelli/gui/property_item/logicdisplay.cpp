/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/property_item/logicdisplay.h"
#include "intelli/gui/style.h"

#include <gt_colors.h>
#include <gt_application.h>

#include <QMouseEvent>
#include <QPainter>

using namespace intelli;

LogicDisplayWidget::LogicDisplayWidget(bool value, QWidget* parent) :
    QWidget(parent),
    m_value(value)
{
    setFixedSize(24, 24);
}

bool
LogicDisplayWidget::value() const
{
    return m_value;
}

void
LogicDisplayWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool
LogicDisplayWidget::readOnly() const
{
    return m_readOnly;
}

void
LogicDisplayWidget::toogle()
{
    setValue(!value());
}

void
LogicDisplayWidget::setValue(bool value)
{
    if (m_value == value) return;

    m_value = value;
    update();
    emit valueChanged(m_value);
}

void
LogicDisplayWidget::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);

    if (!readOnly()) toogle();
}

void
LogicDisplayWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Define the circle's position and size
    double circleRadius = qMin(width(), height()) * 0.5;
    double x = (width() - circleRadius);
    double y = (height() - circleRadius);

    QColor brushColor = value() ? Qt::green : Qt::white;
    QColor outlineColor = readOnly() ? gt::gui::color::disabled() : Qt::black;

    painter.setPen(Qt::NoPen);
    painter.setBrush(outlineColor);
    painter.drawEllipse(QPointF{x, y}, circleRadius, circleRadius);

    circleRadius -= 2 * (readOnly() ? 0.5 : 1);
    painter.setBrush(brushColor);
    painter.drawEllipse(QPointF{x, y},
                        circleRadius, circleRadius);
}
