/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHSCENEMANAGER_H
#define GT_INTELLI_GRAPHSCENEMANAGER_H

#include <intelli/memory.h>
#include <intelli/globals.h>

#include <QObject>
#include <QPointer>

namespace intelli
{

class Graph;
class GraphScene;
class GraphView;

/**
 * @brief The GraphSceneManager class. Handles the creation and lifetime of
 * scenes and can be used to easily switch between scenes.
 */
class GraphSceneManager : public QObject
{
    Q_OBJECT

public:

    GraphSceneManager(GraphView& view);

    /**
     * @brief Creates a scene manager object for the view. Ownership is
     * taken care of. The scene manager can be used to easily switch between
     * scenes and to cleanup scenes once the view is destroyed.
     * @param view View to create scene manager for.
     * @return Pointer to scene manager.
     */
    static GraphSceneManager* make(GraphView& view);

    GraphScene* createScene(Graph& graph);

private:

    QPointer<GraphView> m_view;

    std::vector<unique_qptr<GraphScene, DirectDeleter>> m_scenes;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENEMANAGER_H
