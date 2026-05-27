/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_OBJECTINPUTNODEUI_H
#define GT_INTELLI_OBJECTINPUTNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtPropertyObjectLinkEditor;

namespace intelli
{

class ObjectInputNode;
class NodeGraphicsObject;

class ObjectInputNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ObjectInputNodeWidget(ObjectInputNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateScope();
    void updateText();

private:

    ObjectInputNode* m_node{};
    GtPropertyObjectLinkEditor* m_editor{};
};

class ObjectInputNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE ObjectInputNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTINPUTNODEUI_H
