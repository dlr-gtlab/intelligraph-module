/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_STRINGINPUTNODEUI_H
#define GT_INTELLI_STRINGINPUTNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtLineEdit;

namespace intelli
{

class StringInputNode;
class NodeGraphicsObject;

class StringInputNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit StringInputNodeWidget(StringInputNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateNodeFromText();
    void updateTextFromNode();

private:

    StringInputNode* m_node{};
    GtLineEdit* m_edit{};
};

class StringInputNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE StringInputNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGINPUTNODEUI_H
