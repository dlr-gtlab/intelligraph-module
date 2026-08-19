/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/widgets/commentstyledialog.h"

#include <intelli/gui/style.h>
#include <intelli/gui/icons.h>
#include <intelli/utilities.h>
#include <intelli/gui/commentdata.h>

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

template<typename Reciever, typename Slot>
ColorButton*
makeColorButton(Reciever* reciever, Slot slot,
               QVector<ColorButton*>& buttons,
               QButtonGroup* group,
               QColor const& baseColor)
{
    constexpr int size = 32;
    auto* button = new ColorButton{baseColor};
    button->setFixedSize(size, size);
    button->setCheckable(true);
    group->addButton(button);
    buttons.push_back(button);

    QObject::connect(button, &QPushButton::clicked, reciever, slot(baseColor));

    return button;
}

QPushButton*
makeAlignmentButton(CommentData& comment, Qt::Alignment alignment)
{
    constexpr int size = 32;
    auto* button = new QPushButton{};
    button->setFixedSize(size, size);
    button->setCheckable(true);
    button->setFlat(true);

    switch (alignment & Qt::AlignHorizontal_Mask)
    {
    case Qt::AlignLeft:
        button->setToolTip(QObject::tr("Left-align text"));
        button->setIcon(gt::gui::icon::intelli::textAlignLeft());
        break;
    case Qt::AlignRight:
        button->setToolTip(QObject::tr("Right-align text"));
        button->setIcon(gt::gui::icon::intelli::textAlignRight());
        break;
    case Qt::AlignHCenter:
        button->setToolTip(QObject::tr("Center text"));
        button->setIcon(gt::gui::icon::intelli::textAlignCenter());
        break;
    }

    auto updateButtonState = [&comment, button, alignment]{
        bool matches = (comment.textAlignment() & Qt::AlignHorizontal_Mask) ==
                       (alignment & Qt::AlignHorizontal_Mask);
        button->setChecked(matches);
    };

    auto updateAlignment = [&comment, alignment]{
        comment.setTextAlignment(alignment);
    };

    QObject::connect(&comment, &CommentData::commentAlignmentChanged,
                     button, updateButtonState);

    QObject::connect(button, &QPushButton::clicked,
                     &comment, updateAlignment);

    updateButtonState();

    return button;
}

inline void
colorizeIcon(QPushButton* button, QColor color)
{
    double max = std::numeric_limits<uint8_t>::max();
    bool dark = color.black() * (color.alpha() / max) > (max / 3);
    button->setIcon(gt::gui::colorize(gt::gui::icon::dots(), dark ? Qt::white : Qt::black));
};

CommentStyleDialog::CommentStyleDialog(CommentData& comment) :
    m_comment(&comment),
    m_customBgColorButton(nullptr),
    m_customTextColorButton(nullptr),
    m_customTextColor(gt::gui::color::text()),
    m_customBgColor(gt::gui::color::base())
{
    setWindowTitle(tr("Edit Comment Style"));
    setWindowIcon(gt::gui::icon::palette() );
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QPalette const& darkTheme = gt::gui::darkTheme();
    QPalette const& lightTheme = gt::gui::standardTheme();

    // background color buttons
    auto* bgColorLabel = new QLabel{tr("Background Color:")};
    auto* bgColorButtonLayout = new QGridLayout;
    auto* bgButtonGroup = new QButtonGroup(this);
    bgButtonGroup->setExclusive(true);

    auto updateBgColor = [&comment](QColor const& color){
        return [&comment, color](){ comment.setBackgroundColor(color.name(QColor::HexArgb)); };
    };
    auto setDefaultBgColor = [&comment](QColor const&){
        return [&comment](){ comment.setBackgroundColor(QString{}); };
    };
    auto setCustomBgColor = [this](QColor const&){
        return [this](){
            getCustomBackgroundColor();
            assert(m_comment);
            m_comment->setBackgroundColor(m_customBgColor.name(QColor::HexArgb));
            m_customBgColorButton->setPrimaryColor(m_customBgColor);
            colorizeIcon(m_customBgColorButton, m_customBgColor);
        };
    };

    auto updateTextColor = [&comment](QColor const& color){
        return [&comment, color](){ comment.setTextColor(color.name(QColor::HexRgb)); };
    };
    auto setDefaultTextColor = [&comment](QColor const&){
        return [&comment](){ comment.setTextColor(QString{}); };
    };
    auto setCustomTextColor = [this](QColor const&){
        return [this](){
            getCustomTextColor();
            assert(m_comment);
            m_comment->setTextColor(m_customTextColor.name(QColor::HexArgb));
            m_customTextColorButton->setPrimaryColor(m_customTextColor);
            colorizeIcon(m_customTextColorButton, m_customTextColor);
        };
    };

    // theme color
    auto* defaultBgButton = makeColorButton(
        &comment, setDefaultBgColor, m_bgColorButtons, bgButtonGroup, lightTheme.color(QPalette::Base));
    defaultBgButton->setChecked(true);
    defaultBgButton->setSecondaryColor(darkTheme.color(QPalette::Base));
    defaultBgButton->setToolTip(tr("Default theme-based color"));

    // defaults
    auto* blackBgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, Qt::black);
    auto* greyBgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, Qt::gray);
    auto* whiteBgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, Qt::white);
    auto* transparentBgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, Qt::transparent);
    // some magic colors
    auto* light1BgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, QColor{QStringLiteral("#fffee5")});
    auto* light2BgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, QColor{QStringLiteral("#d9e2ea")});
    auto* dark1BgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, QColor{QStringLiteral("#132726")});
    auto* dark2BgButton = makeColorButton(
        &comment, updateBgColor, m_bgColorButtons, bgButtonGroup, QColor{QStringLiteral("#1c1c2f")});

    // custom color
    m_customBgColorButton = makeColorButton(
        this, setCustomBgColor, m_bgColorButtons, bgButtonGroup, m_customBgColor);
    m_customBgColorButton->setToolTip(tr("Custom color"));
    colorizeIcon(m_customBgColorButton, gt::gui::color::base());

    int row = 0, col = 0;
    bgColorButtonLayout->addWidget(defaultBgButton, row, col++);
    bgColorButtonLayout->addWidget(blackBgButton, row, col++);
    bgColorButtonLayout->addWidget(greyBgButton, row, col++);
    bgColorButtonLayout->addWidget(whiteBgButton, row, col++);
    bgColorButtonLayout->addWidget(transparentBgButton, row, col++);
    row++;
    col = 0;
    bgColorButtonLayout->addWidget(dark1BgButton, row, col++);
    bgColorButtonLayout->addWidget(dark2BgButton, row, col++);
    bgColorButtonLayout->addWidget(light1BgButton, row, col++);
    bgColorButtonLayout->addWidget(light2BgButton, row, col++);
    bgColorButtonLayout->addWidget(m_customBgColorButton, row, col++);
    auto* stretch = new QSpacerItem{1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding};
    bgColorButtonLayout->addItem(stretch, row, col++);

    // text color buttons
    auto* textColorLabel = new QLabel{tr("Text Color:")};
    auto* textColorButtonLayout = new QHBoxLayout;
    auto* textButtonGroup = new QButtonGroup(this);
    textButtonGroup->setExclusive(true);

    // theme color
    auto* defaultTextColorButton = makeColorButton(
        &comment, setDefaultTextColor, m_textColorButtons, textButtonGroup, lightTheme.color(QPalette::Text));
    defaultTextColorButton->setChecked(true);
    defaultTextColorButton->setSecondaryColor(darkTheme.color(QPalette::Text));
    defaultTextColorButton->setToolTip(defaultBgButton->toolTip());

    // defaults
    auto* blackTextColorButton = makeColorButton(
        &comment, updateTextColor, m_textColorButtons, textButtonGroup, Qt::black);
    auto* greyTextColorButton = makeColorButton(
        &comment, updateTextColor, m_textColorButtons, textButtonGroup, Qt::gray);
    auto* whiteTextColorButton = makeColorButton(
        &comment, updateTextColor, m_textColorButtons, textButtonGroup, Qt::white);

    // custom color
    m_customTextColorButton = makeColorButton(
        this, setCustomTextColor, m_textColorButtons, textButtonGroup, m_customTextColor);
    m_customTextColorButton->setToolTip(tr("Custom color"));
    colorizeIcon(m_customTextColorButton, gt::gui::color::text());

    textColorButtonLayout->addWidget(defaultTextColorButton);
    textColorButtonLayout->addWidget(blackTextColorButton);
    textColorButtonLayout->addWidget(greyTextColorButton);
    textColorButtonLayout->addWidget(whiteTextColorButton);
    textColorButtonLayout->addWidget(m_customTextColorButton);
    textColorButtonLayout->addStretch(1);

    // frame
    auto* frameLabel = new QLabel{tr("Frame:")};
    auto* showFrameCheckBox = new QCheckBox{tr("Show Frame")};
    showFrameCheckBox->setChecked(m_comment->showFrame());

    connect(showFrameCheckBox, &QCheckBox::stateChanged, &comment, [&comment](int state){
        comment.setShowFrame(state > 0);
    });

    // text alignment
    auto* alignmentLabel = new QLabel{tr("Text Alignment:")};

    auto* alignmentButtonTopLeft   = makeAlignmentButton(comment, Qt::AlignLeft);
    auto* alignmentButtonTopCenter = makeAlignmentButton(comment, Qt::AlignHCenter);
    auto* alignmentButtonTopRight  = makeAlignmentButton(comment, Qt::AlignRight);

    auto* alignmentButtonLayout = new QHBoxLayout;
    alignmentButtonLayout->addWidget(alignmentButtonTopLeft);
    alignmentButtonLayout->addWidget(alignmentButtonTopCenter);
    alignmentButtonLayout->addWidget(alignmentButtonTopRight);
    alignmentButtonLayout->addStretch(1);

    // dialog buttons
    auto* closeButton = new QPushButton{tr("Close")};
    closeButton->setDefault(true);
    closeButton->setAutoDefault(false);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    // main layout
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(bgColorLabel);
    layout->addLayout(bgColorButtonLayout);
    layout->addWidget(textColorLabel);
    layout->addLayout(textColorButtonLayout);
    layout->addWidget(alignmentLabel);
    layout->addLayout(alignmentButtonLayout);
    layout->addWidget(frameLabel);
    layout->addWidget(showFrameCheckBox);
    layout->addStretch(1);
    layout->addLayout(buttonsLayout);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    bgColorLabel->setMinimumWidth(250);

    connect(closeButton, &QPushButton::clicked, this, &CommentStyleDialog::accept);

    setBackgroundColor(m_comment->backgroundColor());
    setTextColor(m_comment->textColor());
}

CommentStyleDialog::~CommentStyleDialog() = default;


void
CommentStyleDialog::setBackgroundColor(QColor const& color)
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
    m_customBgColor = color;
    emit customBackgroundColorChanged();
}

void
CommentStyleDialog::setTextColor(QColor const& color)
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
    m_customTextColor = color;
    emit customTextColorChanged();
}

void
CommentStyleDialog::getCustomBackgroundColor()
{
    QColor color = QColorDialog::getColor(
        m_customBgColor,
        nullptr,
        tr("Select Custom Background Color"),
        QColorDialog::ShowAlphaChannel
    );

    if (color.isValid())
    {
        m_customBgColor = color;
    }
}

void
CommentStyleDialog::getCustomTextColor()
{
    QColor color = QColorDialog::getColor(
        m_customTextColor,
        nullptr,
        tr("Select Custom Text Color")
    );

    if (color.isValid())
    {
        m_customTextColor = color;
    }
}
