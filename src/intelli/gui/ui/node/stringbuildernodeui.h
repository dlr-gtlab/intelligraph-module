/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_STRINGBUILDERNODEUI_H
#define GT_INTELLI_STRINGBUILDERNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtLineEdit;

namespace intelli
{

class StringBuilderNode;
class NodeGraphicsObject;

class StringBuilderNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit StringBuilderNodeWidget(StringBuilderNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updatePatternFromNode();
    void updateNodeFromPattern();

private:

    StringBuilderNode* m_node{};
    GtLineEdit* m_edit{};
};

class StringBuilderNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE StringBuilderNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGBUILDERNODEUI_H
