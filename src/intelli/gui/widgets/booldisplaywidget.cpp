/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/widgets/booldisplaywidget.h>
#include <intelli/gui/style.h>

#include <gt_finally.h>
#include <gt_colors.h>
#include <gt_palette.h>

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <cmath>

using namespace intelli;

BoolDisplayWidget::BoolDisplayWidget(bool value, DisplayMode mode, QWidget* parent) :
    QWidget(parent),
    m_value(value)
{
    applyDisplayMode(mode);
}

void
BoolDisplayWidget::setDisplayMode(DisplayMode mode)
{
    if (m_mode == mode) return;

    applyDisplayMode(mode);
}

void
BoolDisplayWidget::applyDisplayMode(DisplayMode mode)
{
    m_mode = mode;

    QSize size{16, 16};

    switch (mode)
    {
    case Button:
        size = QSize{24, 24};
        break;
    case Checkbox:
    default:
        break;
    }

    // setFixedSize does not work propertly
    setMinimumSize(size);
    setMaximumSize(size);

    // resize next frame (allows size hint to be calculated correctly)
    QTimer::singleShot(0, this, [this](){ resize(minimumSizeHint()); });
}


BoolDisplayWidget::DisplayMode
BoolDisplayWidget::displayMode() const
{
    return m_mode;
}

void
BoolDisplayWidget::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly) return;

    m_readOnly = readOnly;
    update();
}

bool
BoolDisplayWidget::readOnly()
{
    return m_readOnly;
}

void
BoolDisplayWidget::toogle()
{
    setValue(!value());
}

bool
BoolDisplayWidget::value()
{
    return m_value;
}

void
BoolDisplayWidget::setValue(bool value)
{
    if (m_value == value) return;

    m_value = value;
    update();
    emit valueChanged(m_value);
}

void
BoolDisplayWidget::mousePressEvent(QMouseEvent* event)
{
    if (readOnly()) return;

    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        update();
    }
}

void
BoolDisplayWidget::mouseReleaseEvent(QMouseEvent* event)
{
    auto cleanup = gt::finally([this](){
        m_pressed = false;
        update();
    });
    Q_UNUSED(cleanup);

    if (readOnly()) return;

    if (!rect().contains(event->localPos().toPoint())) return;

    if (event->button() == Qt::LeftButton)
    {
        toogle();
    }
}

void
BoolDisplayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int size = qMin(width(), height());

    switch (m_mode)
    {
    default:
    case Checkbox:
    {
        constexpr int penSize = 2;

        QColor fillColor = gt::gui::color::main();
        QColor outlineColor = gt::gui::color::text();

        // lighten color if currently pressed
        if (m_pressed)
        {
            constexpr int tintModifier = 15;
            fillColor = style::tint(fillColor, tintModifier);
        }

        QPen pen = outlineColor;
        pen.setWidth(penSize);

        // draw outline
        painter.setPen(pen);
        painter.setBrush(fillColor);
        painter.drawRect(0, 0, size, size);

        // draw state
        if (value())
        {
            pen.setWidth(pen.width());
            painter.setPen(pen);

            const int pad = pen.width() * 2;
            QPoint topLeft     = {0    + pad, 0    + pad};
            QPoint topRight    = {size - pad, 0    + pad};
            QPoint bottomLeft  = {0    + pad, size - pad};
            QPoint bottomRight = {size - pad, size - pad};

            painter.drawLine(topLeft, bottomRight);
            painter.drawLine(bottomLeft, topRight);
        }

        break;
    }
    case Button:
    {
        constexpr int penSize = 1;

        int circleRadius = std::floor(size * 0.5) - penSize;
        int x = (width()  - circleRadius) - penSize;
        int y = (height() - circleRadius) - penSize;

        QColor fillColor = value() ?
                               Qt::green :
                               palette().color(QPalette::Base);
        QColor outlineColor = value() ? Qt::black : Qt::gray;

        QPen pen = outlineColor;
        pen.setWidth(penSize);
        // lighten color if currently pressed
        if (m_pressed)
        {
            constexpr int tintModifier = 15;
            fillColor = style::tint(fillColor, tintModifier * (value() ? -1 : 1));
        }

        // draw outline
        painter.setPen(pen);
        painter.setBrush(fillColor);
        painter.drawEllipse(QPoint{x, y}, circleRadius, circleRadius);

        break;
    }
    }
}
