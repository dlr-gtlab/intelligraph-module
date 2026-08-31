/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_STRINGSELECTIONNODEUI_H
#define GT_INTELLI_STRINGSELECTIONNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QComboBox>

namespace intelli
{

class StringSelectionNode;
class NodeGraphicsObject;

class StringSelectionNodeWidget : public QComboBox
{
    Q_OBJECT

public:

    explicit StringSelectionNodeWidget(StringSelectionNode& node);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateOptionsFromNode();
    void updateSelectionFromNode();
    void updateNodeFromSelection(QString const& text);

private:

    StringSelectionNode* m_node{};
};

class StringSelectionNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE StringSelectionNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGSELECTIONNODEUI_H
