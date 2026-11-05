/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/fileinputnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/gui/utilities.h>
#include <intelli/node/input/fileinput.h>

#include <gt_abstractproperty.h>
#include <gt_filedialog.h>
#include <gt_openfilenameproperty.h>
#include <gt_propertyfilechoosereditor.h>

#include <QApplication>
#include <QGraphicsWidget>
#include <QLayout>
#include <QPushButton>

using namespace intelli;

FileInputNodeWidget::FileInputNodeWidget(FileInputNode& node, QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_editor = new GtPropertyFileChooserEditor(this);
    lay->addWidget(m_editor);

    m_editor->setMinimumWidth(150);

    m_fileProp = new GtOpenFileNameProperty("ui_file",
                                            QObject::tr("File"),
                                            QObject::tr("File Path"),
                                            QStringList{});
    m_fileProp->setParent(m_editor);
    m_fileProp->setVal(m_node->selectedFile());
    m_editor->setFileChooserProperty(m_fileProp);

    QObject::connect(m_node, &FileInputNode::fileNameInputConnectionChanged,
                     this, &FileInputNodeWidget::updateWidgetVisibility);
    QObject::connect(m_node, &FileInputNode::selectedFileChanged,
                     this, &FileInputNodeWidget::syncPropertyFromNode);
    QObject::connect(m_fileProp, &GtAbstractProperty::changed,
                     this, &FileInputNodeWidget::syncNodeFromProperty);

    updateWidgetVisibility(m_node->isFileNameInputConnected());
    updateSelectButton();
}

NodeUI::QGraphicsWidgetPtr
FileInputNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<FileInputNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<FileInputNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
FileInputNodeWidget::updateWidgetVisibility(bool connected)
{
    m_editor->setVisible(!connected);
    setMinimumWidth(connected ? 10 : m_editor->minimumWidth());
    adjustSize();
}

void
FileInputNodeWidget::syncPropertyFromNode(QString const& path)
{
    if (m_fileProp->get() == path) return;
    m_fileProp->setVal(path);
}

void
FileInputNodeWidget::syncNodeFromProperty()
{
    m_node->setSelectedFile(m_fileProp->get());
}

void
FileInputNodeWidget::chooseFile()
{
    QWidget* dialogParent = QApplication::activeWindow();
    if (!dialogParent) dialogParent = this;

    auto const fileName = GtFileDialog::getOpenFileName(
        dialogParent,
        QObject::tr("Choose File"),
        m_node->dialogDirectory());
    if (fileName.isEmpty()) return;

    m_fileProp->setVal(fileName);
}

void
FileInputNodeWidget::updateSelectButton()
{
    auto const& btns = m_editor->findChildren<QPushButton*>();
    if (btns.empty()) return;

    // In GtPropertyFileChooserEditor, the select button is added last.
    QPushButton* btn = btns.last();
    if (!btn) return;

    btn->disconnect();
    QObject::connect(btn, &QPushButton::clicked,
                     this, &FileInputNodeWidget::chooseFile);
}

FileInputNodeUI::FileInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
FileInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<FileInputNode const*>(&n)) return {};

    return &FileInputNodeWidget::create;
}
