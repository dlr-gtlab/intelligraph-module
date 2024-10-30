/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHSCENESELECTOR_H
#define GT_INTELLI_GRAPHSCENESELECTOR_H

#include <QWidget>
#include <QPointer>

class QLabel;

namespace intelli
{

class Graph;

/**
 * @brief The GraphSceneSelector class. Widget that displays the hierarchy
 * of the current scene as a path. Can be used to traverse between graph levels.
 */
class GraphSceneSelector : public QWidget
{
    Q_OBJECT

public:

    GraphSceneSelector(QWidget* parent = nullptr);

    /**
     * @brief Refreshes the scene path of the widget. Must be called once the
     * scene changes
     * @param graph Current graph
     */
    void setCurrentGraph(Graph& graph);

public slots:

    void clear();

    void refresh();

signals:

    void graphClicked(QString const& graphUuid);

private:

    QPointer<Graph> m_currentGraph;
    QLabel* m_scenePath;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENESELECTOR_H
