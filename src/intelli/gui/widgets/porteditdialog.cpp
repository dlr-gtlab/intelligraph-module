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

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>

using namespace intelli;

struct PortEditDialog::Impl
{
    /// maps type-id to type names, used for displaying easy to read type names
    /// instead of raw type id
    QHash<TypeId, TypeName> typeToName;

    QPushButton* portCaptionVisibleBtn{};
    QLineEdit* portCaptionEdit{};
    QComboBox* portTypeComboBox{};
    QCheckBox* listTypeCheckBox{};
    QCheckBox* portOptionalCheckBox{};

    TypeId typeId{};
    QString caption{};
    bool captionVisible = true;
    bool hideListTypes = false;
};

PortEditDialog::PortEditDialog(PortType portType,
                               QStringList const& typeIdWhiteList,
                               Options options) :
    pimpl(std::make_unique<Impl>())
{
    setWindowTitle(portType == PortType::In ?
                       tr("Edit Input Port") : tr("Edit Output Port"));
    setWindowIcon(gt::gui::icon::config());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto& factory = NodeDataFactory::instance();

    auto isListType = [&](TypeId const& t){ return factory.isListType(t); };
    pimpl->hideListTypes =
        (std::count_if(typeIdWhiteList.begin(), typeIdWhiteList.end(), isListType) * 2) == typeIdWhiteList.size();

    for (TypeId const& typeId : typeIdWhiteList)
    {
        if (pimpl->hideListTypes && factory.isListType(typeId)) continue;

        TypeName typeName = factory.typeName(typeId);
        if (pimpl->typeToName.find(typeName) != pimpl->typeToName.end())
        {
            typeName = QStringLiteral("%1 (%2)").arg(typeName, typeId);
        }
        pimpl->typeToName.insert(typeId, typeName);
    };

    QStringList typeIds = pimpl->typeToName.values();
    typeIds.sort();

    auto* layout = new QGridLayout();
    auto* portTypeLabel = new QLabel{tr("Port Type:")};

    pimpl->portTypeComboBox = new QComboBox{};
    pimpl->portTypeComboBox->addItems(typeIds);
    pimpl->portTypeComboBox->setToolTip(
        tr("Select the Port Type"));

    auto* portCaptionLabel = new QLabel{tr("Port Caption:")};
    pimpl->portCaptionEdit = new QLineEdit{};
    pimpl->portCaptionEdit->setToolTip(
        tr("Enter Port Caption: %1")
            .arg(gt::rex::onlyLettersAndNumbersAndDotHint()));
    pimpl->portCaptionVisibleBtn = new QPushButton{};
    pimpl->portCaptionVisibleBtn->setCheckable(true);
    pimpl->portCaptionVisibleBtn->setChecked(true);
    pimpl->portCaptionVisibleBtn->setFlat(true);

    auto* listTypeLabel = new QLabel{tr("Use List Type:")};
    pimpl->listTypeCheckBox = new QCheckBox{};
    pimpl->listTypeCheckBox->setChecked(false);
    pimpl->listTypeCheckBox->setToolTip(tr("Activate to generate a list variant"));

    auto* portOptionalLabel = new QLabel{tr("Port Optional:")};
    pimpl->portOptionalCheckBox = new QCheckBox{};
    pimpl->portOptionalCheckBox->setChecked(true);
    pimpl->portOptionalCheckBox->setToolTip(
        tr("Denotes whether the node may start evaluation if "
           "the port is not connected"));

    // apply options
    if (options.testFlag(DisallowEditingCaption))
    {
        pimpl->portCaptionEdit->setEnabled(false);
    }
    if (options.testFlag(DisallowEditingOptional))
    {
        pimpl->portCaptionVisibleBtn->setEnabled(false);
    }

    if (!pimpl->hideListTypes)
    {
        pimpl->listTypeCheckBox->hide();
        listTypeLabel->hide();
    }

    constexpr int rowSpan = 1;
    constexpr int colSpan = 3;

    auto* captionLayout = new QHBoxLayout;
    captionLayout->setContentsMargins(0, 0, 0, 0);
    captionLayout->addWidget(pimpl->portCaptionEdit);
    captionLayout->addWidget(pimpl->portCaptionVisibleBtn);

    int row = 1;
    layout->addWidget(portTypeLabel, row, 1);
    layout->addWidget(pimpl->portTypeComboBox, row++, 2, rowSpan, 2);
    layout->addWidget(listTypeLabel, row, 1);
    layout->addWidget(pimpl->listTypeCheckBox, row++, 2, rowSpan, 2);
    layout->addWidget(portCaptionLabel, row, 1);
    layout->addWidget(pimpl->portCaptionEdit, row, 2);
    layout->addWidget(pimpl->portCaptionVisibleBtn, row++, 3);
    layout->addWidget(portOptionalLabel, row, 1);
    layout->addWidget(pimpl->portOptionalCheckBox, row++, 2);

    // dialog buttons
    auto applyButton = new QPushButton{tr("Apply")};
    applyButton->setIcon(gt::gui::icon::save());
    applyButton->setDefault(true);
    applyButton->setAutoDefault(false);

    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line, row++, 1, rowSpan, colSpan);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(applyButton);
    layout->addLayout(buttonsLayout, row++, 2, rowSpan, colSpan - 1);

    setLayout(layout);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(applyButton, &QPushButton::clicked, this, &QDialog::accept);

    auto updateTypeId = [this, &factory](QString const& currentText){
        // update typeId
        pimpl->typeId = pimpl->typeToName.key(currentText);
        if (pimpl->listTypeCheckBox->isChecked())
        {
            TypeId listType = factory.listType(pimpl->typeId);
            pimpl->typeId = std::move(listType);
        }

        // update caption edit
        TypeName typeName = factory.typeName(pimpl->typeId);
        pimpl->portCaptionEdit->setPlaceholderText(typeName);
        if (pimpl->portCaptionEdit->text().isEmpty())
        {
            pimpl->caption = std::move(typeName);
        }
    };

    auto updateCaption = [this, &factory](QString const& currentText){
        pimpl->caption = currentText;
        if (currentText.isEmpty())
        {
            TypeName typeName = factory.typeName(pimpl->typeId);
            pimpl->caption = std::move(typeName);
        }
    };

    auto updateCaptionVisibility = [this](bool checked){
        pimpl->captionVisible = checked;
        pimpl->portCaptionVisibleBtn->setIcon(
            checked ? gt::gui::icon::eye() : gt::gui::icon::eyeOff());
        pimpl->portCaptionVisibleBtn->setToolTip(
            checked ? tr("Port caption is visible") : tr("Port caption is hidden"));
    };

    auto updatePortOptional = [=](bool checked){
        pimpl->portOptionalCheckBox->setText(
            checked ? tr("optional") : tr("required"));
    };
    auto updateListBtn = [=](bool checked){
        pimpl->listTypeCheckBox->setText(checked ? tr("on") : tr("off"));
        updateTypeId(pimpl->portTypeComboBox->currentText());
    };

    connect(pimpl->portTypeComboBox, &QComboBox::currentTextChanged,
            this, updateTypeId);
    connect(pimpl->portCaptionEdit, &QLineEdit::textChanged,
            this, updateCaption);
    connect(pimpl->portCaptionVisibleBtn, &QPushButton::clicked,
            this, updateCaptionVisibility);
    connect(pimpl->listTypeCheckBox, &QCheckBox::checkStateChanged,
            this, updateListBtn);
    connect(pimpl->portOptionalCheckBox, &QCheckBox::checkStateChanged,
            this, updatePortOptional);

    pimpl->portTypeComboBox->setCurrentText(pimpl->typeToName[intelli::typeId<DoubleData>()]);

    updateListBtn(pimpl->listTypeCheckBox->isChecked());
    updateCaptionVisibility(pimpl->portCaptionVisibleBtn->isChecked());
    updatePortOptional(true);

    // invalid inputs -> abort dialog
    if (typeIds.empty())
    {
        gtWarning() << tr("Failed to edit port data, invalid type ids!");
        QTimer::singleShot(0, this, &QDialog::reject);
    }
    // nothing to select
    if (typeIds.size() == 1)
    {
        pimpl->portTypeComboBox->setEnabled(false);
    }
}

PortEditDialog::~PortEditDialog() = default;

void
PortEditDialog::setTypeId(TypeId const& typeId)
{
    auto& factory = NodeDataFactory::instance();

    TypeId tmpTypeId = typeId;
    if (pimpl->hideListTypes && factory.isListType(typeId))
    {
        tmpTypeId = factory.innerType(tmpTypeId);
        pimpl->listTypeCheckBox->setChecked(true);
    }

    pimpl->portTypeComboBox->setCurrentText(pimpl->typeToName[tmpTypeId]);
    emit pimpl->portTypeComboBox->currentTextChanged(pimpl->portTypeComboBox->currentText());
    setCaption(pimpl->caption);
}

void
PortEditDialog::setCaption(const QString& caption)
{
    auto& factory = NodeDataFactory::instance();

    TypeId typeId = pimpl->typeToName.key(pimpl->portTypeComboBox->currentText());
    if (pimpl->listTypeCheckBox->isChecked())
    {
        typeId = factory.listType(typeId);
    }

    TypeName typeName = factory.typeName(typeId);
    if (typeName == caption)
    {
        pimpl->portCaptionEdit->clear();
        return;
    }

    pimpl->portCaptionEdit->setText(caption);
}

void
PortEditDialog::setCaptionVisible(bool visible)
{
    pimpl->portCaptionVisibleBtn->setChecked(visible);
    emit pimpl->portCaptionVisibleBtn->clicked(visible);
}

void
PortEditDialog::setOptional(bool optional)
{
    pimpl->portOptionalCheckBox->setChecked(optional);
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

bool
PortEditDialog::optional() const
{
    return pimpl->portOptionalCheckBox->isChecked();
}
