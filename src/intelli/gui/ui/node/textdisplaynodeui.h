/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_TEXTDISPLAYNODEUI_H
#define GT_INTELLI_TEXTDISPLAYNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtCodeEditor;

namespace intelli
{

class TextDisplayNode;
class NodeGraphicsObject;

class TextDisplayNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit TextDisplayNodeWidget(TextDisplayNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateTextFromNode();
    void updateHighlighterFromNode();

private:

    TextDisplayNode* m_node{};
    GtCodeEditor* m_editor{};
};

class TextDisplayNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE TextDisplayNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_TEXTDISPLAYNODEUI_H
