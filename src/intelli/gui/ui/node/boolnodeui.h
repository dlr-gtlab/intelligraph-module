/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLNODEUI_H
#define GT_INTELLI_BOOLNODEUI_H

#include <intelli/gui/nodeui.h>
#include <intelli/gui/widgets/booldisplaygraphicswidget.h>

#include <intelli/node/booldisplay.h>
#include <intelli/node/input/boolinput.h>

namespace intelli
{

class NodeGraphicsObject;

class BoolDisplayNodeWidget : public BoolDisplayGraphicsWidget
{
    Q_OBJECT

public:

    explicit BoolDisplayNodeWidget(BoolDisplayNode& node);
    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateValueFromNode();
    void updateDisplayModeFromNode();

private:

    BoolDisplayNode* m_node = nullptr;
};

class BoolInputNodeWidget : public BoolDisplayGraphicsWidget
{
    Q_OBJECT

public:

    explicit BoolInputNodeWidget(BoolInputNode& node);
    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateNodeValueFromWidget(bool value);
    void updateWidgetValueFromNode();
    void updateDisplayModeFromNode();

private:

    BoolInputNode* m_node = nullptr;
};

class BoolNodeUI : public NodeUI
{
    Q_OBJECT

public:
    
    Q_INVOKABLE BoolNodeUI();
    
    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLNODEUI_H
