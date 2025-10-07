/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "graphuservariablesdialog.h"

#include <intelli/graph.h>
#include <intelli/graphuservariables.h>
#include <intelli/gui/widgets/editablelabel.h>

#include <gt_icons.h>
#include <gt_colors.h>
#include <gt_logging.h>
#include <gt_palette.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QAction>

using namespace intelli;

namespace
{

using TupleType = std::tuple<QString, QIcon, QVariant::Type>;

static QVector<TupleType> const& typeIds()
{
    using namespace gt::gui;
    static QVector<TupleType> types{
        {
            QObject::tr("Boolean"),
            icon::letter::bSmall(),
            QVariant::Bool
        },
        {
            QObject::tr("Integer"),
            icon::letter::iSmall(),
            QVariant::Int
        },
        {
            QObject::tr("Floating Point"),
            icon::letter::fSmall(),
            QVariant::Double
        },
        {
            QObject::tr("String"),
            icon::letter::s(),
            QVariant::String
        },
    };
    return types;
}

template<typename Functor>
static void foreachItem(QListWidget& listView, Functor const& f)
{
    int count = listView.count();
    for (int i = 0; i < count; i++)
    {
        QListWidgetItem* item = listView.item(i);
        if (!item) continue;

        QWidget* widget = listView.itemWidget(item);
        if (!widget) continue;

        auto* uvItem = static_cast<GraphUserVariableItem*>(widget);
        f(uvItem);
    }
}

} // namespace


GraphUserVariablesDialog::GraphUserVariablesDialog(Graph& graph) :
    m_graph(&graph)
{
    setWindowTitle(tr("Edit Graph User Variables"));
    setWindowIcon(gt::gui::icon::config());
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setMinimumWidth(400);
    setMinimumHeight(300);

    auto* layout = new QVBoxLayout();

    // info label
    auto* infoLayout = new QHBoxLayout();
    auto* infoLabel = new QLabel{tr(
        "In the following, variables can be defined that are accessible by "
        "all nodes. These variables can be thought of as static environment "
        "variables, specific to each graph hierarchy. All subgraphs have "
        "access to the same variables."
    )};

    QSize size{16, 16};
    auto* infoIcon = new QLabel();
    infoIcon->setPixmap(gt::gui::icon::info2().pixmap(size));
    infoIcon->setFixedSize(size);

    infoLayout->addWidget(infoIcon);
    infoLayout->addSpacing(4);
    infoLayout->addWidget(infoLabel);

    infoLabel->setWordWrap(true);

    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    // user variables
    m_listView = new QListWidget();

    auto* addVariableButton = new QPushButton{tr("Add")};
    addVariableButton->setIcon(gt::gui::icon::add());
    addVariableButton->setDefault(false);
    addVariableButton->setAutoDefault(false);
    addVariableButton->setToolTip(tr("Add new User Variable"));

    connect(addVariableButton, &QPushButton::pressed, this, [this](){
        addItem(QString{}, QVariant::fromValue(QString{}));
    });

    // dialog buttons
    m_saveButton = new QPushButton{tr("Save")};
    m_saveButton->setIcon(gt::gui::icon::save());
    m_saveButton->setDefault(false);
    m_saveButton->setAutoDefault(false);

    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());
    closeButton->setDefault(false);
    closeButton->setAutoDefault(false);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addWidget(addVariableButton);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(m_saveButton);
    buttonsLayout->addWidget(closeButton);

    layout->addLayout(infoLayout);
    layout->addWidget(m_listView);
    layout->addLayout(buttonsLayout);

    setLayout(layout);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveChanges()));

    load();
}

void
GraphUserVariablesDialog::open()
{
    m_listView->setFocus();
}

GraphUserVariablesDialog::~GraphUserVariablesDialog() = default;

void
GraphUserVariablesDialog::saveChanges()
{
    if (!m_graph)
    {
        gtError() << tr("GraphUserVariables: Could not save user variables, "
                        "graph no longer exists!");
        return;
    }

    auto* uv = m_graph->findDirectChild<GraphUserVariables*>();
    assert(uv);

    QVector<std::pair<QString, QVariant>> keyValueList;

    foreachItem(*m_listView, [&keyValueList](GraphUserVariableItem* item){
        if (!item->isActivated()) return;
        keyValueList.push_back({
            item->key(),
            item->value()
        });
    });

    QStringList exisitingKeys = uv->keys();

    for (auto entry : qAsConst(keyValueList))
    {
        auto const& key = std::get<QString>(entry);
        auto const& value = std::get<QVariant>(entry);

        if (uv->hasValue(key)) exisitingKeys.removeOne(key);

        uv->setValue(key, value);
    }

    for (QString const& oldKey : qAsConst(exisitingKeys))
    {
        uv->remove(oldKey);
    }

    emit uv->variablesUpdated();

    accept();
}

bool
GraphUserVariablesDialog::validate() const
{
    auto const getItemWidget = [this](auto begin, auto iter){
        int idx = std::distance(begin, iter);
        auto* item = static_cast<GraphUserVariableItem*>(
            m_listView->itemWidget(m_listView->item(idx))
        );
        assert(item);
        return item;
    };

    QStringList keys;

    foreachItem(*m_listView, [&keys](GraphUserVariableItem* item){
        keys.push_back(item->key());
    });

    // check if some keys are empty
    bool hasEmptyKey = std::find(keys.cbegin(), keys.cend(), QString{}) != keys.cend();

    // check if keys contain duplicates
    bool areKeysValid = true;
    auto rcurrent = keys.crbegin();
    auto rend  = keys.crend();
    auto begin = keys.cbegin();
    auto end   = keys.cend();
    while (rcurrent != rend)
    {
        auto increment = gt::finally([&](){ ++rcurrent; });
        Q_UNUSED(increment);
        if (rcurrent->isEmpty())
        {
            auto* item = getItemWidget(begin, std::next(rcurrent).base());
            item->setIsDuplicateKey(false);
            continue;
        }

        auto duplicate = std::find(rcurrent.base(), end, *rcurrent);
        if (duplicate == end)
        {
            auto* item = getItemWidget(begin, std::next(rcurrent).base());
            item->setIsDuplicateKey(false);
            continue;
        }

        areKeysValid = false;

        for (auto& iter : { std::next(rcurrent).base(), duplicate })
        {
            auto* item = getItemWidget(begin, iter);
            item->setIsDuplicateKey(true);
        }
    }

    // check if values are valid
    bool areValuesValid = true;
    foreachItem(*m_listView, [&areValuesValid](GraphUserVariableItem* item){
        if (!item->isValid()) areValuesValid = false;
    });

    return areKeysValid && areValuesValid && !hasEmptyKey;
}

void
GraphUserVariablesDialog::load()
{
    assert(m_graph);

    auto* uv = m_graph->findDirectChild<GraphUserVariables*>();
    assert(uv);

    uv->visit([this](QString const& key, QVariant const& value){
        addItem(key, value);
    });
}

void
GraphUserVariablesDialog::addItem(QString key, QVariant value)
{
    auto* itemWidget = new GraphUserVariableItem(key, value);
    auto* item = new QListWidgetItem(m_listView);
    item->setSizeHint(itemWidget->minimumSizeHint());
    m_listView->setItemWidget(item, itemWidget);

    auto const updateSaveButton = [this](){
        m_saveButton->setEnabled(validate());
    };

    connect(itemWidget, &GraphUserVariableItem::keyChanged, this, updateSaveButton);
    connect(itemWidget, &GraphUserVariableItem::valueChanged, this, updateSaveButton);
    connect(itemWidget, &QWidget::destroyed, m_listView, [item, updateSaveButton](){
        delete item;
        updateSaveButton();
    });

    itemWidget->init();
    updateSaveButton();
}

////////////////////////////////////////////////////////////////////////////////

GraphUserVariableItem::GraphUserVariableItem(QString key,
                                             QVariant value,
                                             QWidget* parent) :
    QWidget(parent)
{
    auto* layout = new QHBoxLayout();
    layout->setContentsMargins(4, 1, 1, 1);

    m_enableCheckBox = new QCheckBox();
    m_enableCheckBox->setChecked(true);
    m_enableCheckBox->setToolTip("Save variable");

    m_keyLabel = new EditableLabel();
    m_keyLabel->setPlaceholderText(tr("<key>"));
    m_keyLabel->edit()->setValidator(new QRegExpValidator(QRegExp("(\\w|\\d)*"), this));

    m_valueEdit = new QLineEdit();
    m_valueEdit->setPlaceholderText(tr("<value>"));

    m_typeComboBox = new QComboBox();
    m_typeComboBox->setToolTip(tr("Variable datatype"));

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QWidget::customContextMenuRequested, this, [this](){
        QMenu menu;
        QAction* deleteAction = menu.addAction(tr("Delete"));
        deleteAction->setIcon(gt::gui::icon::delete_());

        if (deleteAction != menu.exec(QCursor::pos())) return;

        delete this;
    });

    auto const& types = typeIds();
    int idx = 0;
    for (auto const& entry : types)
    {
        m_typeComboBox->addItem(std::get<QString>(entry));
        m_typeComboBox->setItemIcon(idx, std::get<QIcon>(entry));
        idx++;
    }

    // apply key and value
    if (!key.isEmpty())
    {
        m_keyLabel->setText(key);
    }
    if (value.isValid())
    {
        auto iter = std::find_if(types.begin(), types.end(), [type = value.type()](auto const& tuple){
            return std::get<QVariant::Type>(tuple) == type;
        });
        if (iter != types.end())
        {
            m_valueEdit->setText(value.toString());
            m_typeComboBox->setCurrentIndex(std::distance(types.begin(), iter));
        }
    }

    m_keyLabel->setMaximumWidth(150);
    m_keyLabel->setMaximumHeight(m_valueEdit->sizeHint().height());

    layout->addWidget(m_enableCheckBox);
    layout->addWidget(m_keyLabel);
    layout->addWidget(m_valueEdit);
    layout->addWidget(m_typeComboBox);

    connect(m_valueEdit, &QLineEdit::textChanged,
            this, &GraphUserVariableItem::valueChanged);
    connect(m_typeComboBox, &QComboBox::currentTextChanged,
            this, &GraphUserVariableItem::valueChanged);
    connect(m_keyLabel, &EditableLabel::textChanged,
            this, &GraphUserVariableItem::keyChanged);

    connect(this, &GraphUserVariableItem::valueChanged,
            this, &GraphUserVariableItem::onValueChanged);

    setLayout(layout);
}

bool
GraphUserVariableItem::isActivated() const
{
    return m_enableCheckBox->isChecked();
}

QString
GraphUserVariableItem::key() const
{
    return m_keyLabel->text();
}

QVariant
GraphUserVariableItem::value() const
{
    int index = m_typeComboBox->currentIndex();

    auto const& types = typeIds();
    if (index < 0 || index >= types.size()) return {};

    QVariant::Type type = std::get<QVariant::Type>(types.at(index));
    QVariant variant{m_valueEdit->text()};

    switch(type)
    {
    // bool is not checked correctly (always yields true)
    case QVariant::Bool:
    {
        QString const& typeName = variant.toString();
        if (typeName.compare("true",  Qt::CaseInsensitive) &&
            typeName.compare("false", Qt::CaseInsensitive))
        {
            return {};
        }
        variant.convert(type);
        break;
    }
    default:
        variant.convert(type);
        break;
    }
    return variant;
}

bool
GraphUserVariableItem::isValid() const
{
    return !value().isNull();
}

void
GraphUserVariableItem::init()
{
    // must be called after widget has been added to avoid resetting palette
    // changes
    onValueChanged();
    setIsDuplicateKey(false);
}

void
GraphUserVariableItem::setIsDuplicateKey(bool isDuplicate)
{
    QPalette palette = m_keyLabel->palette();
    palette.setColor(QPalette::Text,
                     isDuplicate ? gt::gui::color::warningText() :
                                   gt::gui::color::text());
    m_keyLabel->edit()->setPalette(palette);

    if (!isDuplicate && key().isEmpty())
    {
        palette.setColor(QPalette::Text, gt::gui::color::disabled());
        palette.setColor(QPalette::WindowText, gt::gui::color::disabled());
    }
    m_keyLabel->setPalette(palette);
    m_keyLabel->label()->setPalette(std::move(palette));

    m_keyLabel->setToolTip(
        isDuplicate ? QStringLiteral("Duplicate variable name") :
                      QStringLiteral("Variable name"));
}

void
GraphUserVariableItem::onValueChanged()
{
    bool isValid = this->isValid();

    QPalette palette = m_valueEdit->palette();
    palette.setColor(QPalette::Text,
                     isValid ? gt::gui::color::text() :
                         gt::gui::color::warningText());
    m_valueEdit->setPalette(std::move(palette));
    m_valueEdit->setToolTip(
        isValid ? QStringLiteral("Variable value") :
                  QStringLiteral("Variable value is incompatible\n"
                                 "with the selected datatype"));
}
