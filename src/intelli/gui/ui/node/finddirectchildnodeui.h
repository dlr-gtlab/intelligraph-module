/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_FINDDIRECTCHILDNODEUI_H
#define GT_INTELLI_FINDDIRECTCHILDNODEUI_H

#include <intelli/gui/nodeui.h>
#include <intelli/gui/widgets/finddirectchildwidget.h>

namespace intelli
{

class FindDirectChildNode;
class NodeGraphicsObject;

class FindDirectChildNodeWidget : public FindDirectChildWidget
{
    Q_OBJECT

public:

    explicit FindDirectChildNodeWidget(FindDirectChildNode& node);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateNameCompleterFromNode();

private:

    FindDirectChildNode* m_node{};
};

class FindDirectChildNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE FindDirectChildNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_FINDDIRECTCHILDNODEUI_H
