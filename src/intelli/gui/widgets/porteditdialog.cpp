/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/widgets/porteditdialog.h"

#include "intelli/data/double.h"
#include "intelli/nodedatafactory.h"

#include <gt_icons.h>
#include <gt_regularexpression.h>
#include <gt_regexpvalidator.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

using namespace intelli;

struct PortEditDialog::Impl
{
    QCheckBox* portCaptionCheckBox{};
    QLineEdit* portCaptionEdit{};
    QComboBox* portTypeComboBox{};

    TypeId typeId{};
    QString caption{};
    bool captionVisible = true;
};

PortEditDialog::PortEditDialog(PortType portType, QStringList const& typeIdWhiteList) :
    pimpl(std::make_unique<Impl>())
{
    setWindowTitle(portType == PortType::In ?
                       tr("Edit Input Port") : tr("Edit Output Port"));
    setWindowIcon(gt::gui::icon::config());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QStringList typeIds = typeIdWhiteList.empty() ?
                                 NodeDataFactory::instance().validTypeIds() :
                                 std::move(typeIdWhiteList);
    typeIds.sort();

    auto* layout = new QGridLayout();
    auto* portTypeLabel = new QLabel{tr("Port Type:")};

    pimpl->portTypeComboBox = new QComboBox{};
    pimpl->portTypeComboBox->addItems(typeIds);

    pimpl->portCaptionCheckBox = new QCheckBox{};
    pimpl->portCaptionEdit = new QLineEdit{};
    pimpl->portCaptionEdit->setValidator(new QRegularExpressionValidator(
        gt::rex::onlyLettersAndNumbersAndDot(), this));

    pimpl->portTypeComboBox->setToolTip(tr("Select the Port Type"));
    pimpl->portCaptionEdit->setToolTip(tr("Enter Port Caption: ") +
                                gt::rex::onlyLettersAndNumbersAndDotHint());
    pimpl->portCaptionCheckBox->setToolTip(tr("Toggle whether caption should be displayed"));

    int row = 1;
    layout->addWidget(portTypeLabel, row, 1);
    layout->addWidget(pimpl->portTypeComboBox, row++, 2);
    layout->addWidget(pimpl->portCaptionCheckBox, row, 1);
    layout->addWidget(pimpl->portCaptionEdit, row++, 2);

    // dialog buttons
    auto applyButton = new QPushButton{tr("Apply")};
    applyButton->setIcon(gt::gui::icon::save());
    applyButton->setDefault(true);
    applyButton->setAutoDefault(false);

    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(applyButton);
    layout->addLayout(buttonsLayout, row++, 1, 1, 2);

    setLayout(layout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);

    auto updateTypeId = [this](QString const& currentText){
        pimpl->typeId = currentText;

        QString typeName = NodeDataFactory::instance()
                               .typeName(currentText);
        pimpl->portCaptionEdit->setPlaceholderText(typeName);

        if (pimpl->portCaptionEdit->text().isEmpty())
        {
            pimpl->caption = std::move(typeName);
        }
    };

    auto updateCaption = [this](QString const& currentText){
        pimpl->caption = currentText;
        if (currentText.isEmpty())
        {
            QString typeName = NodeDataFactory::instance()
                                   .typeName(pimpl->portTypeComboBox->currentText());
            pimpl->caption = std::move(typeName);
        }
    };

    auto updateCaptionVisibility = [this](bool checked){
        pimpl->captionVisible = checked;
        pimpl->portCaptionCheckBox->setText(checked ? tr("true") : tr("false"));
    };

    connect(pimpl->portTypeComboBox, &QComboBox::currentTextChanged,
            this, updateTypeId);
    connect(pimpl->portCaptionEdit, &QLineEdit::textChanged,
            this, updateCaption);
    connect(pimpl->portCaptionCheckBox, &QCheckBox::clicked,
            this, updateCaptionVisibility);

    pimpl->portTypeComboBox->setCurrentText(intelli::typeId<DoubleData>());
    pimpl->portCaptionCheckBox->setChecked(true);

    // invalid inputs -> abort dialog
    if (typeIds.empty())
    {
        gtWarning() << tr("Failed to edit port data, invalid type ids!");
        reject();
    }
    // nothing to select
    if (typeIds.size() == 1)
    {
        pimpl->portTypeComboBox->setEnabled(false);
    }
}

PortEditDialog::~PortEditDialog() = default;

void
PortEditDialog::setTypeId(const TypeId& typeId)
{
    pimpl->portTypeComboBox->setCurrentText(typeId);
}

void
PortEditDialog::setCaption(const QString& caption)
{
    pimpl->portCaptionEdit->setText(caption.trimmed());
}

void
PortEditDialog::setCaptionVisible(bool visible)
{
    pimpl->portCaptionCheckBox->setChecked(visible);
}

TypeId
PortEditDialog::typeId() const
{
    return pimpl->typeId;
}

QString
PortEditDialog::caption() const
{
    return pimpl->caption;
}

bool
PortEditDialog::captionVisible() const
{
    return pimpl->captionVisible;
}

