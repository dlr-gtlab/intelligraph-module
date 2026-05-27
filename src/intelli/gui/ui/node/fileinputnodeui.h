/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_FILEINPUTNODEUI_H
#define GT_INTELLI_FILEINPUTNODEUI_H

#include <intelli/gui/nodeui.h>
#include <QWidget>

class GtOpenFileNameProperty;
class GtPropertyFileChooserEditor;

namespace intelli
{

class FileInputNode;
class NodeGraphicsObject;

class FileInputNodeWidget : public QWidget
{
    Q_OBJECT

public:

    explicit FileInputNodeWidget(FileInputNode& node, QWidget* parent = nullptr);

    static NodeUI::QGraphicsWidgetPtr create(Node& source, NodeGraphicsObject& object);

private slots:

    void updateWidgetVisibility(bool connected);
    void syncPropertyFromNode(QString const& path);
    void syncNodeFromProperty();
    void chooseFile();

private:

    void updateSelectButton();

    FileInputNode* m_node{};
    GtPropertyFileChooserEditor* m_editor{};
    GtOpenFileNameProperty* m_fileProp{};
};

class FileInputNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE FileInputNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_FILEINPUTNODEUI_H
