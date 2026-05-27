/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_NUMBERMATHNODEUI_H
#define GT_INTELLI_NUMBERMATHNODEUI_H

#include <intelli/gui/nodeui.h>
#include <intelli/node/numbermath.h>
#include <QWidget>

class QComboBox;

namespace intelli
{

class NumberMathNode;
class NodeGraphicsObject;

class NumberMathNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit NumberMathNodeWidget(NumberMathNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateComboFromNode();
    void updateNodeFromCombo(QString const& text);

private:

    static QString toText(NumberMathNode::MathOperation op);
    static NumberMathNode::MathOperation fromText(QString const& text);

    NumberMathNode* m_node{};
    QComboBox* m_combo{};
};

class NumberMathNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE NumberMathNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERMATHNODEUI_H
