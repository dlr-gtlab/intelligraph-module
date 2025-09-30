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

class GraphUserVariableItem : public QWidget
{
public:

    GraphUserVariableItem(QString key, QVariant value, QWidget* parent = nullptr) :
        QWidget(parent)
    {
        auto* layout = new QHBoxLayout();
        layout->setContentsMargins(4, 1, 1, 1);

        m_enableCheckBox = new QCheckBox();
        m_enableCheckBox->setChecked(true);
        m_enableCheckBox->setToolTip("Enable Variable");

        m_keyLabel = new EditableLabel(tr("<key>"));
        m_keyLabel->setToolTip(tr("Variable Name"));

        m_valueEdit = new QLineEdit();
        m_valueEdit->setPlaceholderText(tr("<value>"));
        m_valueEdit->setToolTip(tr("Variable Value"));

        m_typeComboBox = new QComboBox();
        m_typeComboBox->setToolTip(tr("Variable Type"));

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
        if (!value.isNull() && !key.isEmpty())
        {
            auto iter = std::find_if(types.begin(), types.end(), [type = value.type()](auto const& tuple){
                return std::get<QVariant::Type>(tuple) == type;
            });
            if (iter != types.end())
            {
                m_valueEdit->setText(value.toString());
                m_keyLabel->setText(key);
                m_typeComboBox->setCurrentIndex(std::distance(types.begin(), iter));
            }
        }

        m_keyLabel->setMaximumWidth(150);
        m_keyLabel->setMaximumHeight(m_valueEdit->sizeHint().height());

        layout->addWidget(m_enableCheckBox);
        layout->addWidget(m_keyLabel);
        layout->addWidget(m_valueEdit);
        layout->addWidget(m_typeComboBox);

        setLayout(layout);
    }

    bool isActivated() const { return m_enableCheckBox->isChecked(); }

    QString key() const { return m_keyLabel->text(); }

    QVariant value() const
    {
        int index = m_typeComboBox->currentIndex();

        auto const& types = typeIds();
        if (index < 0 || index > types.size()) return {};

        QVariant variant{m_valueEdit->text()};
        variant.convert(std::get<QVariant::Type>(types.at(index)));
        return variant;
    }

private:

    QCheckBox* m_enableCheckBox{};
    EditableLabel* m_keyLabel{};
    QLineEdit* m_valueEdit{};
    QComboBox* m_typeComboBox{};

    using TupleType = std::tuple<QString, QIcon, QVariant::Type>;

    static QVector<TupleType> const& typeIds()
    {
        using namespace gt::gui;
        static QVector<TupleType> types{
            {
                tr("Boolean"),
                icon::letter::bSmall(),
                QVariant::Bool
            },
            {
                tr("Integer"),
                icon::letter::iSmall(),
                QVariant::Int
            },
            {
                tr("Floating Point"),
                icon::letter::fSmall(),
                QVariant::Double
            },
            {
                tr("String"),
                icon::letter::s(),
                QVariant::String
            },
        };
        return types;
    }
};

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

    connect(addVariableButton, &QPushButton::pressed,
            this, [this](){
        addItem("name", 0);
    });

    // dialog buttons
    auto* saveButton = new QPushButton{tr("Save")};
    saveButton->setIcon(gt::gui::icon::save());
    auto* closeButton = new QPushButton{tr("Cancel")};
    closeButton->setIcon(gt::gui::icon::cancel());

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(4, 4, 4, 4);
    buttonsLayout->addWidget(addVariableButton);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(closeButton);

    layout->addLayout(infoLayout);
    layout->addWidget(m_listView);
    layout->addLayout(buttonsLayout);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveChanges()));

    setLayout(layout);

    load();
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

    int count = m_listView->count();
    for (int i = 0; i < count; i++)
    {
        QListWidgetItem* item = m_listView->item(i);
        if (!item) continue;

        QWidget* widget = m_listView->itemWidget(item);
        if (!widget) continue;

        auto* uvItem = static_cast<GraphUserVariableItem*>(widget);
        if (!uvItem->isActivated()) continue;

        keyValueList.push_back({
            uvItem->key(),
            uvItem->value()
        });
    }

    gtDebug() << "SAVING" << keyValueList;

    accept();
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

    connect(itemWidget, &QWidget::destroyed, m_listView, [item](){
        delete item;
    });
}
