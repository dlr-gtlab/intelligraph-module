/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_OBJECTSINKNODEUI_H
#define GT_INTELLI_OBJECTSINKNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class QPushButton;

namespace intelli
{

class ObjectSink;
class NodeGraphicsObject;

class ObjectSinkNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit ObjectSinkNodeWidget(ObjectSink& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateExportEnabled(bool enabled);
    void exportObject();

private:

    ObjectSink* m_node{};
    QPushButton* m_button{};
};

class ObjectSinkNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE ObjectSinkNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTSINKNODEUI_H
