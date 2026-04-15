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

FileInputNodeUI::FileInputNodeUI() = default;

NodeUI::WidgetFactoryFunction
FileInputNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<FileInputNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<FileInputNode*>(&source);
        if (!node) return nullptr;

        auto b = utils::makeWidgetWithLayout();
        auto* lay = b->layout();

        auto* w = new GtPropertyFileChooserEditor();
        lay->addWidget(w);

        w->setMinimumWidth(150);

        auto* fileProp = new GtOpenFileNameProperty("ui_file",
                                                    QObject::tr("File"),
                                                    QObject::tr("File Path"),
                                                    QStringList{});
        fileProp->setParent(w);
        fileProp->setVal(node->selectedFile());
        w->setFileChooserProperty(fileProp);

        auto const updateWidget = [w, b_ = b.get()](bool connected) {
            w->setVisible(!connected);
            b_->setMinimumWidth(connected ? 10 : w->minimumWidth());
            b_->resize(b_->minimumSize());
        };

        QObject::connect(node, &FileInputNode::fileNameInputConnectionChanged, w, updateWidget);
        QObject::connect(node, &FileInputNode::selectedFileChanged, w, [fileProp](QString const& path) {
            if (fileProp->get() == path) return;
            fileProp->setVal(path);
        });
        QObject::connect(fileProp, &GtAbstractProperty::changed, w, [node, fileProp]() {
            node->setSelectedFile(fileProp->get());
        });
        updateWidget(node->isFileNameInputConnected());

        auto const& btns = w->findChildren<QPushButton*>();
        if (!btns.empty())
        {
            // Override the editor's select-button handler so we can choose
            // a stable top-level parent for the dialog in the graphics scene.
            // In GtPropertyFileChooserEditor, the select button is added last.
            QPushButton* btn = btns.last();

            if (btn)
            {
                btn->disconnect();
                QObject::connect(btn, &QPushButton::clicked, w, [node, w, fileProp]() {
                    QWidget* dialogParent = QApplication::activeWindow();
                    if (!dialogParent) dialogParent = w;

                    auto const fileName = GtFileDialog::getOpenFileName(dialogParent,
                                                                        QObject::tr("Choose File"),
                                                                        node->dialogDirectory());
                    if (fileName.isEmpty()) return;

                    fileProp->setVal(fileName);
                });
            }
        }

        return convertToGraphicsWidget(std::move(b), object);
    };
}
