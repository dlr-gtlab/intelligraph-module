/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/existingdirectorysourcenodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/existingdirectorysource.h>

#include <gt_abstractproperty.h>
#include <gt_existingdirectoryproperty.h>
#include <gt_filedialog.h>

#include <QApplication>
#include <QGraphicsWidget>
#include <QPushButton>

using namespace intelli;

ExistingDirectorySourceNodeWidget::ExistingDirectorySourceNodeWidget(ExistingDirectorySourceNode& node) :
    GtPropertyFileChooserEditor()
{
    m_node = &node;

    setMinimumWidth(120);

    // Keep a dedicated UI property so the editor can own lifetime/updates.
    m_property = new GtExistingDirectoryProperty("ui_directory",
                                                  QObject::tr("Directory"),
                                                  QObject::tr("Directory"));
    m_property->setParent(this);
    m_property->setVal(m_node->directory());
    setFileChooserProperty(m_property);

    QObject::connect(m_node, &ExistingDirectorySourceNode::directoryChanged,
                     this, &ExistingDirectorySourceNodeWidget::syncPropertyFromNode);
    QObject::connect(m_property, &GtAbstractProperty::changed,
                     this, &ExistingDirectorySourceNodeWidget::syncNodeFromProperty);

    updateSelectButton();
}

NodeUI::QGraphicsWidgetPtr
ExistingDirectorySourceNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<ExistingDirectorySourceNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<ExistingDirectorySourceNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
ExistingDirectorySourceNodeWidget::syncPropertyFromNode(QString const& path)
{
    if (m_property->get() == path) return;
    m_property->setVal(path);
}

void
ExistingDirectorySourceNodeWidget::syncNodeFromProperty()
{
    m_node->setDirectory(m_property->get());
}

void
ExistingDirectorySourceNodeWidget::chooseDirectory()
{
    QWidget* dialogParent = QApplication::activeWindow();
    if (!dialogParent) dialogParent = this;

    auto const directory = GtFileDialog::getExistingDirectory(
        dialogParent,
        QObject::tr("Choose Directory"),
        m_node->directory());
    if (directory.isEmpty()) return;

    m_property->setVal(directory);
}

void
ExistingDirectorySourceNodeWidget::updateSelectButton()
{
    auto const& buttons = findChildren<QPushButton*>();
    if (buttons.empty()) return;

    // Override the editor's select-button handler so we can choose
    // a stable top-level parent for the dialog in the graphics scene.
    QPushButton* btn = buttons.last();
    if (!btn) return;

    btn->disconnect();
    QObject::connect(btn, &QPushButton::clicked,
                     this, &ExistingDirectorySourceNodeWidget::chooseDirectory);
}

ExistingDirectorySourceNodeUI::ExistingDirectorySourceNodeUI() = default;

NodeUI::WidgetFactoryFunction
ExistingDirectorySourceNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ExistingDirectorySourceNode const*>(&n)) return {};

    return &ExistingDirectorySourceNodeWidget::create;
}
