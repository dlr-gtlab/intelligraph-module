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

#include <gt_colors.h>
#include <gt_icons.h>
#include <gt_stylesheets.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>

using namespace intelli;


struct CommentColorDialog::Impl
{

static QString
buttonStylesheet(QColor const& baseColor, bool selected = false)
{
    using namespace gt::gui;

    return QStringLiteral(
               "QAbstractButton{"
               " border: 1px solid %2;"
               " border-radius: 3px;"
               " background-color: %1;"
               "}"
               "QAbstractButton:hover{"
               " border: 1px solid %3;"
               "}"
               ).arg(
            baseColor.name(),
            selected ?
                color::highlight().name() :
                color::darken(baseColor, 15).name(),
            color::lighten(color::highlight(), 20).name());
}

static QPushButton*
makeColorButton(CommentColorDialog* dialog, QColor* target, QColor const& baseColor, bool selected = false)
{
    constexpr int size = 32;
    auto* button = new QPushButton{};
    button->setFixedSize(size, size);
    button->setStyleSheet(buttonStylesheet(baseColor, selected));

    connect(button, &QPushButton::clicked, dialog, [baseColor, dialog, target](){
        *target = QColorDialog::getColor(
            baseColor,
            nullptr,
            tr("Select Background Color"),
            target == &dialog->m_bgColor ?
                QColorDialog::ShowAlphaChannel :
                QColorDialog::ColorDialogOptions{}
        );
    });

    return button;
}

}; // struct Impl

CommentColorDialog::CommentColorDialog() :
    m_showBorderCheckBox(nullptr),
    m_textColor(gt::gui::color::text()),
    m_bgColor(gt::gui::color::base())
{
    setWindowTitle(tr("Edit Comment Style"));
    setWindowIcon(gt::gui::icon::palette() );
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setMinimumWidth(400);
//    setMinimumHeight(300);

    auto* bgColorLabel = new QLabel{tr("Background Color:")};
    auto* textColorLabel = new QLabel{tr("Text Color:")};
    m_showBorderCheckBox = new QCheckBox{tr("Show Border")};
    m_showBorderCheckBox->setChecked(true);

    auto* bgColorButtonLayout = new QHBoxLayout;
    auto* defaultBackgroundColorButton = Impl::makeColorButton(this, &m_bgColor, gt::gui::color::base(), true);
    auto* blackBackgroundColorButton = Impl::makeColorButton(this, &m_bgColor, Qt::black);
    auto* greyBackgroundColorButton = Impl::makeColorButton(this, &m_bgColor, Qt::gray);
    auto* whiteBackgroundColorButton = Impl::makeColorButton(this, &m_bgColor, Qt::white);
    auto* customBgColorButton = Impl::makeColorButton(this, &m_bgColor, gt::gui::color::base());
    customBgColorButton->setIcon(gt::gui::icon::dots());

    bgColorButtonLayout->addWidget(defaultBackgroundColorButton);
    bgColorButtonLayout->addWidget(blackBackgroundColorButton);
    bgColorButtonLayout->addWidget(greyBackgroundColorButton);
    bgColorButtonLayout->addWidget(whiteBackgroundColorButton);
    bgColorButtonLayout->addWidget(customBgColorButton);
    bgColorButtonLayout->addStretch(1);

    auto* textColorButtonLayout = new QHBoxLayout;
    auto* defaultTextColorButton = Impl::makeColorButton(this, &m_textColor, gt::gui::color::text(), true);
    auto* invertedTextColorButton = Impl::makeColorButton(this, &m_textColor, style::invert(gt::gui::color::text()));
    auto* customTextColorButton = Impl::makeColorButton(this, &m_textColor, style::invert(gt::gui::color::text()));
    customTextColorButton->setIcon(gt::gui::icon::dots());

    textColorButtonLayout->addWidget(defaultTextColorButton);
    textColorButtonLayout->addWidget(invertedTextColorButton);
    textColorButtonLayout->addWidget(customTextColorButton);
    textColorButtonLayout->addStretch(1);

    // dialog buttons
    auto* saveButton = new QPushButton{tr("Save")};
    saveButton->setIcon(gt::gui::icon::save());
    saveButton->setDefault(false);
    saveButton->setAutoDefault(false);

    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(closeButton);

    auto* layout = new QVBoxLayout();
    layout->addWidget(bgColorLabel);
    layout->addLayout(bgColorButtonLayout);
    layout->addWidget(textColorLabel);
    layout->addLayout(textColorButtonLayout);
    layout->addWidget(m_showBorderCheckBox);
    layout->addStretch(1);
    layout->addLayout(buttonsLayout);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    connect(closeButton, &QPushButton::clicked, this, &CommentColorDialog::reject);
    connect(saveButton, &QPushButton::clicked, this, [this](){
        if (!textColor().isValid() || !backgroundColor().isValid())
        {
            return reject();
        }
        accept();
    });
}

void
CommentColorDialog::setShowBorder(bool enable)
{
    assert(m_showBorderCheckBox);
    m_showBorderCheckBox->setChecked(enable);
}

QColor
CommentColorDialog::textColor() const
{
    return m_textColor;
}

QColor
CommentColorDialog::backgroundColor() const
{
    return m_bgColor;
}

bool
CommentColorDialog::showBorder() const
{
    assert(m_showBorderCheckBox);
    return m_showBorderCheckBox->isChecked();
}

CommentColorDialog::~CommentColorDialog() = default;

