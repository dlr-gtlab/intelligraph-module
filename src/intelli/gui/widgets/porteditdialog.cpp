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
    QPushButton* listTypeBtn{};
    QCheckBox* portOptionalCheckBox{};

    TypeId typeId{};
    QString caption{};
    bool captionVisible = true;
    bool allowLists = false;
};

PortEditDialog::PortEditDialog(PortType portType,
                               Option option,
                               QStringList const& typeIdWhiteList) :
    pimpl(std::make_unique<Impl>())
{

    setWindowTitle(portType == PortType::In ?
                       tr("Edit Input Port") : tr("Edit Output Port"));
    setWindowIcon(gt::gui::icon::config());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto& factory = NodeDataFactory::instance();

    QStringList typeIds = typeIdWhiteList.empty() ?
                                 factory.validTypeIds() :
                                 std::move(typeIdWhiteList);

    if (option == AllowListTypes)
    {
        pimpl->allowLists = true;

        if (!typeIdWhiteList.isEmpty())
        {
            // remove list types
            auto isListType = std::bind(
                &NodeDataFactory::isListType, &factory, std::placeholders::_1);
            typeIds.erase(std::remove_if(typeIds.begin(),
                                         typeIds.end(),
                                         isListType),
                          typeIds.end());
        }
    }

    for (auto typeId : typeIds)
    {
        TypeName typeName = NodeDataFactory::instance().typeName(typeId);
        if (pimpl->typeToName.find(typeName) != pimpl->typeToName.end())
        {
            typeName = QStringLiteral("%1 (%2)").arg(typeName, typeId);
        }
        pimpl->typeToName.insert(typeId, typeName);
    };

    typeIds = pimpl->typeToName.values();
    typeIds.sort();

    auto* layout = new QGridLayout();
    auto* portTypeLabel = new QLabel{tr("Port Type:")};

    pimpl->portTypeComboBox = new QComboBox{};
    pimpl->portTypeComboBox->addItems(typeIds);

    auto* portCaptionLabel = new QLabel{tr("Port Caption:")};
    pimpl->portCaptionVisibleBtn = new QPushButton{};
    pimpl->listTypeBtn = new QPushButton{};
    pimpl->portCaptionEdit = new QLineEdit{};

    pimpl->portTypeComboBox->setToolTip(
        tr("Select the Port Type"));

    pimpl->portCaptionEdit->setToolTip(
        tr("Enter Port Caption: %1")
            .arg(gt::rex::onlyLettersAndNumbersAndDotHint()));

    pimpl->portCaptionVisibleBtn->setCheckable(true);
    pimpl->portCaptionVisibleBtn->setChecked(true);
    pimpl->portCaptionVisibleBtn->setFlat(true);

    pimpl->listTypeBtn->setCheckable(true);
    pimpl->listTypeBtn->setChecked(false);
    pimpl->listTypeBtn->setIcon(gt::gui::icon::list());
    pimpl->listTypeBtn->setToolTip(tr("Activate to generate list variant"));
    pimpl->listTypeBtn->setFlat(true);
    pimpl->listTypeBtn->setVisible(pimpl->allowLists);

    auto* portOptionalLabel = new QLabel{tr("Port Optional:")};
    pimpl->portOptionalCheckBox = new QCheckBox{};
    pimpl->portOptionalCheckBox->setChecked(true);
    pimpl->portOptionalCheckBox->setToolTip(
        tr("Denotes whether the node may start evaluation if "
           "the port is not connected"));

    constexpr int rowSpan = 1;
    constexpr int colSpan = 3;

    auto* captionLayout = new QHBoxLayout;
    captionLayout->setContentsMargins(0, 0, 0, 0);
    captionLayout->addWidget(pimpl->portCaptionEdit);
    captionLayout->addWidget(pimpl->portCaptionVisibleBtn);

    int row = 1;
    layout->addWidget(portTypeLabel, row, 1);
    layout->addWidget(pimpl->portTypeComboBox, row, 2, rowSpan, 1 + !pimpl->allowLists);
    layout->addWidget(pimpl->listTypeBtn, row++, 3);
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

    auto updateTypeId = [this](QString const& currentText){
        auto& factory = NodeDataFactory::instance();

        pimpl->typeId = pimpl->typeToName.key(currentText);
        if (pimpl->listTypeBtn->isChecked())
        {
            pimpl->typeId = factory.listType(pimpl->typeId);
        }

        TypeName typeName = factory.typeName(pimpl->typeId);
        pimpl->portCaptionEdit->setPlaceholderText(typeName);

        if (pimpl->portCaptionEdit->text().isEmpty())
        {
            pimpl->caption = std::move(typeName);
        }
    };

    auto updateCaption = [this](QString const& currentText){
        auto& factory = NodeDataFactory::instance();

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

    connect(pimpl->portTypeComboBox, &QComboBox::currentTextChanged,
            this, updateTypeId);
    connect(pimpl->portCaptionEdit, &QLineEdit::textChanged,
            this, updateCaption);
    connect(pimpl->portCaptionVisibleBtn, &QPushButton::clicked,
            this, updateCaptionVisibility);
    connect(pimpl->listTypeBtn, &QPushButton::clicked,
            this, [=](){ updateTypeId(pimpl->portTypeComboBox->currentText()); });
    connect(pimpl->portOptionalCheckBox, &QPushButton::clicked,
            this, updatePortOptional);

    pimpl->portTypeComboBox->setCurrentText(pimpl->typeToName[intelli::typeId<DoubleData>()]);
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
    if (pimpl->allowLists && factory.isListType(typeId))
    {
        tmpTypeId = factory.innerType(tmpTypeId);
        pimpl->listTypeBtn->setChecked(true);
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
    if (pimpl->listTypeBtn->isChecked())
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
