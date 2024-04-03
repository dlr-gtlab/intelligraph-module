/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 20.10.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/property_item/logic.h"
#include "gt_logging.h"

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
    int circleSize = qMin(width(), height()) - 2;
    int x = (width() - circleSize) * 0.5;
    int y = (height() - circleSize) * 0.5;

    painter.setPen(QPen(Qt::black, readOnly() ? 0 : 2));

    if (value())
    {
        painter.setBrush(QBrush(Qt::green));
    }
    else
    {
        painter.setBrush(QBrush(Qt::white));
    }

    painter.drawEllipse(x, y, circleSize, circleSize);
}
