/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_GENERICCALCULATOREXECNODEUI_H
#define GT_INTELLI_GENERICCALCULATOREXECNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtPropertyTreeView;
class QComboBox;

namespace intelli
{

class GenericCalculatorExecNode;
class NodeGraphicsObject;

class GenericCalculatorExecNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit GenericCalculatorExecNodeWidget(GenericCalculatorExecNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateCurrentObjectView();
    void updateClassFromEditor(QString const& classText);
    void updateClassEditorFromNode(QString const& className);

private:

    GenericCalculatorExecNode* m_node{};
    QComboBox* m_classEdit{};
    GtPropertyTreeView* m_view{};
};

class GenericCalculatorExecNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GenericCalculatorExecNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_GENERICCALCULATOREXECNODEUI_H
