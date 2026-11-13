/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHSCENEMENU_H
#define GT_INTELLI_GRAPHSCENEMENU_H

#include <QMenu>

class QLineEdit;
class QTreeWidget;

namespace intelli
{

/// Menu for adding nodes and comments in a graoh scene
class GraphSceneMenu : public QMenu
{
    Q_OBJECT
public:

    explicit GraphSceneMenu(QWidget* parent = nullptr);

protected:

    void keyPressEvent(QKeyEvent* event) override;

signals:

    /// user requested the creation of a comment object
    void createCommentRequested();

    /// user requested the creation of a node of the given type
    void createNodeRequested(QString className);

private:

    QLineEdit* m_filterEdit{};

    QTreeWidget* m_treeView{};

    bool m_guard{false};
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENEMENU_H
