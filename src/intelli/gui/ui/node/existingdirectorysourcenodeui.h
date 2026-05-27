/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_EXISTINGDIRECTORYSOURCENODEUI_H
#define GT_INTELLI_EXISTINGDIRECTORYSOURCENODEUI_H

#include <intelli/gui/nodeui.h>
#include <gt_propertyfilechoosereditor.h>

class GtExistingDirectoryProperty;

namespace intelli
{

class ExistingDirectorySourceNode;
class NodeGraphicsObject;

class ExistingDirectorySourceNodeWidget : public GtPropertyFileChooserEditor
{
    Q_OBJECT

public:

    explicit ExistingDirectorySourceNodeWidget(ExistingDirectorySourceNode& node);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void syncPropertyFromNode(QString const& path);
    void syncNodeFromProperty();
    void chooseDirectory();

private:

    void updateSelectButton();

    ExistingDirectorySourceNode* m_node{};
    GtExistingDirectoryProperty* m_property{};
};

class ExistingDirectorySourceNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE ExistingDirectorySourceNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_EXISTINGDIRECTORYSOURCENODEUI_H
