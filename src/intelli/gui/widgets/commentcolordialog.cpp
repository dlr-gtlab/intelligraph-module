/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/widgets/commentcolordialog.h"

#include <intelli/gui/style.h>
#include <intelli/utilities.h>

#include <gt_colors.h>
#include <gt_icons.h>
#include <gt_stylesheets.h>
#include <gt_palette.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QPainter>
#include <QPainterPath>
#include <QColorDialog>

using namespace intelli;

ColorButton::ColorButton(QColor primary, QColor secondary, QWidget* parent) :
    m_primary(primary),
    m_secondary(secondary)
{ }

void
ColorButton::setPrimaryColor(QColor color)
{
    m_primary = std::move(color);
    update();
}

QColor ColorButton::primaryColor() const { return m_primary; }

void
ColorButton::setSecondaryColor(QColor color)
{
    m_secondary = std::move(color);
    update();
}

QColor ColorButton::secondaryColor() const { return m_secondary; }

void
ColorButton::paintEvent(QPaintEvent* e)
{
    constexpr int r = 6;
    bool isHovered = underMouse();

    QRect rect = this->rect();
    QRect adjustedRect = rect.adjusted(1, 1, -1, -1);

    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing, false);

    // background
    QPainterPath clip;
    clip.addRoundedRect(adjustedRect, r, r);
    painter.setClipPath(clip);
    painter.setClipping(true);

    if (m_primary.alpha() < static_cast<int>(std::numeric_limits<uint8_t>::max()))
    {
        // draw checker pattern for transparency
        const int cell = 5;
        for (int y = 1; y < rect.height(); y += cell)
        {
            for (int x = 1; x < rect.width(); x += cell)
            {
                bool isAlternate = ((x / cell) + (y / cell)) % 2 == 0;
                painter.fillRect(x, y, cell ,cell, isAlternate ? Qt::white : Qt::lightGray);
            }
        }
    }

    if (m_secondary.isValid())
    {
        QPainterPath top{rect.topLeft()};
        top.lineTo(rect.topRight());
        top.lineTo(rect.bottomLeft());
        painter.setBrush(m_primary);
        painter.drawPath(top);

        QPainterPath bottom{rect.bottomLeft()};
        bottom.lineTo(rect.topRight());
        bottom.lineTo(rect.bottomRight());
        painter.setBrush(m_secondary);
        painter.drawPath(bottom);
    }
    else
    {
        painter.fillRect(rect, m_primary);
    }

    // outline
    painter.setClipping(false);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen;
    pen.setWidth(isChecked() ? 2 : 1);
    if (isChecked())
    {
        pen.setColor(isHovered ?
                         style::tint(gt::gui::color::highlight(), 30) :
                         gt::gui::color::highlight());
    }
    else
    {
        pen.setColor(isHovered ?
                         style::tint(gt::gui::color::highlight(), -30) :
                         gt::gui::color::lightFrame());
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(pen);
    painter.drawRoundedRect(adjustedRect, r, r);

    // icon
    QIcon icon = this->icon();
    if (icon.isNull()) return;

    QSize iconSize = this->iconSize();
    if (!iconSize.isValid()) return;

    QRect iconRect{
       rect.center() - QPointF{std::ceil(iconSize.width() * 0.5) - 1,
                               std::ceil(iconSize.height() * 0.5) - 1}.toPoint(),
       iconSize
    };

    icon.paint(&painter, iconRect);
}

ColorButton*
makeColorButton(QVector<ColorButton*>& buttons, QButtonGroup* group, QColor const& baseColor)
{
    constexpr int size = 32;
    auto* button = new ColorButton{baseColor};
    button->setFixedSize(size, size);
    button->setCheckable(true);
    group->addButton(button);
    buttons.push_back(button);

    return button;
}

QPushButton*
makeAlignmentButton(QButtonGroup* group, Qt::Alignment alignment = {})
{
    constexpr int size = 32;
    auto* button = new QPushButton{};
    button->setFixedSize(size, size);
    button->setCheckable(true);

    QPixmap pixmap = gt::gui::icon::arrowUp().pixmap(size, size);
    QTransform transform;

    switch (alignment)
    {
    case Qt::AlignLeft  | Qt::AlignTop:
        transform.rotate(-45);
        break;
    case Qt::AlignRight | Qt::AlignTop:
        transform.rotate(45);
        break;
    case Qt::AlignLeft  | Qt::AlignVCenter:
        transform.rotate(-90);
        break;
    case Qt::AlignRight | Qt::AlignVCenter:
        transform.rotate(90);
        break;
    case Qt::AlignCenter:
        pixmap = gt::gui::icon::square().pixmap(size, size);
        break;
    case Qt::AlignLeft  | Qt::AlignBottom:
        transform.rotate(-135);
        break;
    case Qt::AlignBottom:
        transform.rotate(180);
        break;
    case Qt::AlignRight | Qt::AlignBottom:
        transform.rotate(135);
        break;
    }

    QPixmap rotatedPixmap = pixmap.transformed(transform, Qt::SmoothTransformation);
    button->setIcon(QIcon{rotatedPixmap});
    button->setFlat(true);
    group->addButton(button);

    return button;
}

CommentColorDialog::CommentColorDialog() :
    m_customBgColorButton(nullptr),
    m_customTextColorButton(nullptr),
    m_showFrameCheckBox(nullptr),
    m_customTextColor(gt::gui::color::text()),
    m_customBgColor(gt::gui::color::base())
{
    setWindowTitle(tr("Edit Comment Style"));
    setWindowIcon(gt::gui::icon::palette() );
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto colorizeIcon = [](QPushButton* button, QColor color){
        double max = std::numeric_limits<uint8_t>::max();
        bool dark = color.black() * (color.alpha() / max) > (max / 3);
        button->setIcon(gt::gui::colorize(gt::gui::icon::dots(), dark ? Qt::white : Qt::black));
    };

    // background color buttons
    auto* bgColorLabel = new QLabel{tr("Background Color:")};
    auto* bgColorButtonLayout = new QHBoxLayout;
    auto* bgButtonGroup = new QButtonGroup(this);
    bgButtonGroup->setExclusive(true);

    QPalette const& darkTheme = gt::gui::darkTheme();
    QPalette const& lightTheme = gt::gui::standardTheme();

    auto* defaultBackgroundColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, lightTheme.color(QPalette::Base));
    defaultBackgroundColorButton->setChecked(true);
    defaultBackgroundColorButton->setSecondaryColor(darkTheme.color(QPalette::Base));
    defaultBackgroundColorButton->setToolTip(tr("Default theme-based color"));

    auto* blackBackgroundColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, Qt::black);
    auto* greyBackgroundColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, Qt::gray);
    auto* whiteBackgroundColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, Qt::white);

    auto* transparentBackgroundColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, Qt::transparent);
    m_customBgColorButton = makeColorButton(m_bgColorButtons, bgButtonGroup, m_customBgColor);
    colorizeIcon(m_customBgColorButton, gt::gui::color::base());

    bgColorButtonLayout->addWidget(defaultBackgroundColorButton);
    bgColorButtonLayout->addWidget(blackBackgroundColorButton);
    bgColorButtonLayout->addWidget(greyBackgroundColorButton);
    bgColorButtonLayout->addWidget(whiteBackgroundColorButton);
    bgColorButtonLayout->addWidget(transparentBackgroundColorButton);
    bgColorButtonLayout->addWidget(m_customBgColorButton);
    bgColorButtonLayout->addStretch(1);

    m_customBgColorButton->disconnect(this);
    connect(m_customBgColorButton, &QPushButton::clicked,
            this, [this](){
        QColor color = QColorDialog::getColor(
            m_customBgColor,
            nullptr,
            tr("Select Background Color"),
            QColorDialog::ShowAlphaChannel
        );
        if (color.isValid())
        {
            m_customBgColor = color;
            emit customBackgroundColorChanged();
        }
    });

    connect(this, &CommentColorDialog::customBackgroundColorChanged,
            this, [colorizeIcon, this](){
        m_customBgColorButton->setPrimaryColor(m_customBgColor);
        colorizeIcon(m_customBgColorButton, m_customBgColor);
    });

    // text color buttons
    auto* textColorLabel = new QLabel{tr("Text Color:")};
    auto* textColorButtonLayout = new QHBoxLayout;
    auto* textButtonGroup = new QButtonGroup(this);
    textButtonGroup->setExclusive(true);

    auto* defaultTextColorButton = makeColorButton(m_textColorButtons, textButtonGroup, lightTheme.color(QPalette::Text));
    defaultTextColorButton->setChecked(true);
    defaultTextColorButton->setSecondaryColor(darkTheme.color(QPalette::Text));
    defaultTextColorButton->setToolTip(defaultBackgroundColorButton->toolTip());

    auto* blackTextColorButton = makeColorButton(m_textColorButtons, textButtonGroup, Qt::black);
    auto* greyTextColorButton = makeColorButton(m_textColorButtons, textButtonGroup, Qt::gray);
    auto* whiteTextColorButton = makeColorButton(m_textColorButtons, textButtonGroup, Qt::white);

    m_customTextColorButton = makeColorButton(m_textColorButtons, textButtonGroup, m_customTextColor);
    colorizeIcon(m_customTextColorButton, gt::gui::color::text());

    textColorButtonLayout->addWidget(defaultTextColorButton);
    textColorButtonLayout->addWidget(blackTextColorButton);
    textColorButtonLayout->addWidget(greyTextColorButton);
    textColorButtonLayout->addWidget(whiteTextColorButton);
    textColorButtonLayout->addWidget(m_customTextColorButton);
    textColorButtonLayout->addStretch(1);

    m_customTextColorButton->disconnect(this);
    connect(m_customTextColorButton, &QPushButton::clicked,
            this, [this](){
        QColor color = QColorDialog::getColor(
            m_customTextColor,
            nullptr,
            tr("Select Text Color")
        );
        if (color.isValid())
        {
            m_customTextColor = color;
            emit customTextColorChanged();
        }
    });

    connect(this, &CommentColorDialog::customTextColorChanged,
            this, [colorizeIcon, this](){
        m_customTextColorButton->setPrimaryColor(m_customTextColor);
        colorizeIcon(m_customTextColorButton, m_customTextColor);
    });

    // frame
    auto* frameLabel = new QLabel{tr("Frame:")};
    m_showFrameCheckBox = new QCheckBox{tr("Show Frame")};
    m_showFrameCheckBox->setChecked(true);

    // alignment
    auto* alignmentLabel = new QLabel{tr("Text Alignment:")};
    auto* alignmentButtonGroup = new QButtonGroup(this);
    alignmentButtonGroup->setExclusive(true);

    auto* alignmentButtonTopLeft      = makeAlignmentButton(alignmentButtonGroup, Qt::AlignTop     | Qt::AlignLeft );
    auto* alignmentButtonTopCenter    = makeAlignmentButton(alignmentButtonGroup, Qt::AlignTop                     );
    auto* alignmentButtonTopRight     = makeAlignmentButton(alignmentButtonGroup, Qt::AlignTop     | Qt::AlignRight);
    auto* alignmentButtonCenterLeft   = makeAlignmentButton(alignmentButtonGroup, Qt::AlignVCenter | Qt::AlignLeft );
    auto* alignmentButtonCenter       = makeAlignmentButton(alignmentButtonGroup, Qt::AlignCenter                  );
    auto* alignmentButtonCenterRight  = makeAlignmentButton(alignmentButtonGroup, Qt::AlignVCenter | Qt::AlignRight);
    auto* alignmentButtonBottomLeft   = makeAlignmentButton(alignmentButtonGroup, Qt::AlignBottom  | Qt::AlignLeft );
    auto* alignmentButtonBottomCenter = makeAlignmentButton(alignmentButtonGroup, Qt::AlignBottom                  );
    auto* alignmentButtonBottomRight  = makeAlignmentButton(alignmentButtonGroup, Qt::AlignBottom  | Qt::AlignRight);
    alignmentButtonTopLeft->setChecked(true);

    auto* alignmentButtonLayout = new QGridLayout;
    int row = 0;
    int col = 0;
    alignmentButtonLayout->addWidget(alignmentButtonTopLeft, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonTopCenter, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonTopRight, row, col);
    row++; col = 0;
    alignmentButtonLayout->addWidget(alignmentButtonCenterLeft, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonCenter, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonCenterRight, row, col);
    row++; col = 0;
    alignmentButtonLayout->addWidget(alignmentButtonBottomLeft, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonBottomCenter, row, col++);
    alignmentButtonLayout->addWidget(alignmentButtonBottomRight, row, col);
    alignmentButtonLayout->addItem(new QSpacerItem{1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding}, row, col+1);

    // dialog buttons
    auto* saveButton = new QPushButton{tr("Apply")};
    saveButton->setIcon(gt::gui::icon::save());
    saveButton->setDefault(true);
    saveButton->setAutoDefault(false);

    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    // main layout
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(bgColorLabel);
    layout->addLayout(bgColorButtonLayout);
    layout->addWidget(textColorLabel);
    layout->addLayout(textColorButtonLayout);
    layout->addWidget(alignmentLabel);
    layout->addLayout(alignmentButtonLayout);
    layout->addWidget(frameLabel);
    layout->addWidget(m_showFrameCheckBox);
    layout->addStretch(1);
    layout->addLayout(buttonsLayout);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    bgColorLabel->setMinimumWidth(300);

    connect(closeButton, &QPushButton::clicked, this, &CommentColorDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, [this](){
        if (!textColor().isValid() || !backgroundColor().isValid())
        {
            return reject();
        }
        accept();
    });
}

CommentColorDialog::~CommentColorDialog() = default;

void
CommentColorDialog::setShowFrame(bool enable)
{
    assert(m_showFrameCheckBox);
    m_showFrameCheckBox->setChecked(enable);
}

bool
CommentColorDialog::showFrame() const
{
    assert(m_showFrameCheckBox);
    return m_showFrameCheckBox->isChecked();
}

bool
CommentColorDialog::isDefaultBackgroundColor() const
{
    assert(m_bgColorButtons.size() > 0);
    ColorButton* defaultBtn = m_bgColorButtons.front();
    assert(defaultBtn);
    return defaultBtn->isChecked();
}

void
CommentColorDialog::setBackgroundColor(QColor const& color)
{
    assert(m_bgColorButtons.size() > 0);
    if (!color.isValid())
    {
        ColorButton* defaultBtn = m_bgColorButtons.front();
        assert(defaultBtn);
        defaultBtn->setChecked(true);
        return;
    }

    for (ColorButton* button :
            utils::makeIterable(std::next(m_bgColorButtons.begin()),
                                m_bgColorButtons.end()))
    {
        if (button->primaryColor() == color)
        {
            button->setChecked(true);
            return;
        }
    }

    ColorButton* customBtn = m_bgColorButtons.back();
    assert(customBtn);
    customBtn->setPrimaryColor(color);
    customBtn->setChecked(true);
    emit customBackgroundColorChanged();
}

QColor
CommentColorDialog::backgroundColor() const
{
    for (ColorButton* button : qAsConst(m_bgColorButtons))
    {
        if (button->isChecked())
        {
            return button->primaryColor();
        }
    }
    return QColor{};
}

bool
CommentColorDialog::isDefaultTextColor() const
{
    assert(m_textColorButtons.size() > 0);
    ColorButton* defaultBtn = m_textColorButtons.front();
    assert(defaultBtn);
    return defaultBtn->isChecked();
}

void
CommentColorDialog::setTextColor(QColor const& color)
{
    assert(m_textColorButtons.size() > 0);
    if (!color.isValid())
    {
        ColorButton* defaultBtn = m_textColorButtons.front();
        assert(defaultBtn);
        defaultBtn->setChecked(true);
        return;
    }

    for (ColorButton* button :
            utils::makeIterable(std::next(m_textColorButtons.begin()),
                                m_textColorButtons.end()))
    {
        if (button->primaryColor() == color)
        {
            button->setChecked(true);
            return;
        }
    }

    ColorButton* customBtn = m_textColorButtons.back();
    assert(customBtn);
    customBtn->setPrimaryColor(color);
    customBtn->setChecked(true);
    emit customTextColorChanged();
}

QColor
CommentColorDialog::textColor() const
{
    for (ColorButton* button : qAsConst(m_textColorButtons))
    {
        if (button->isChecked())
        {
            return button->primaryColor();
        }
    }
    return QColor{};
}

void
CommentColorDialog::setTextAlignment(Qt::Alignment alignment)
{

}

Qt::Alignment
CommentColorDialog::textAlignment() const
{
    return Qt::AlignCenter;
}
