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
#include <gt_propertyfilechoosereditor.h>

#include <QApplication>
#include <QGraphicsWidget>
#include <QPushButton>

using namespace intelli;

ExistingDirectorySourceNodeUI::ExistingDirectorySourceNodeUI() = default;

NodeUI::WidgetFactoryFunction
ExistingDirectorySourceNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<ExistingDirectorySourceNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<ExistingDirectorySourceNode*>(&source);
        if (!node) return nullptr;

        // Editor widget is embedded via QGraphicsProxyWidget.
        auto w = std::make_unique<GtPropertyFileChooserEditor>();
        w->setMinimumWidth(120);

        // Keep a dedicated UI property so the editor can own lifetime/updates.
        auto* prop = new GtExistingDirectoryProperty("ui_directory",
                                                     QObject::tr("Directory"),
                                                     QObject::tr("Directory"));
        prop->setParent(w.get());
        prop->setVal(node->directory());
        w->setFileChooserProperty(prop);

        // Sync node -> UI.
        QObject::connect(node, &ExistingDirectorySourceNode::directoryChanged,
                         w.get(), [prop](QString const& path) {
            if (prop->get() == path) return;
            prop->setVal(path);
        });

        // Sync UI -> node.
        QObject::connect(prop, &GtAbstractProperty::changed,
                         w.get(), [node, prop]() {
            node->setDirectory(prop->get());
        });

        auto const& buttons = w->findChildren<QPushButton*>();
        if (!buttons.empty())
        {
            // Override the editor's select-button handler so we can choose
            // a stable top-level parent for the dialog in the graphics scene.
            QPushButton* btn = buttons.last();

            if (btn)
            {
                btn->disconnect();
                QObject::connect(btn, &QPushButton::clicked, w.get(), [node, w_ = w.get(), prop]() {
                    QWidget* dialogParent = QApplication::activeWindow();
                    if (!dialogParent) dialogParent = w_;

                    auto const directory = GtFileDialog::getExistingDirectory(
                        dialogParent,
                        QObject::tr("Choose Directory"),
                        node->directory());
                    if (directory.isEmpty()) return;

                    prop->setVal(directory);
                });
            }
        }

        return convertToGraphicsWidget(std::move(w), object);
    };
}
