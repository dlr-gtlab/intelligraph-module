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
    ~GraphSceneManager();

    /**
     * @brief Creates a scene manager object for the view. Ownership is
     * taken care of. The scene manager can be used to easily switch between
     * scenes and to cleanup scenes once the view is destroyed.
     * @param view View to create scene manager for.
     * @return Pointer to scene manager.
     */
    static GraphSceneManager* make(GraphView& view);

    /**
     * @brief Returns the current scene of the view.
     * @return Current scene (may be null if no scene was registered)
     */
    GraphScene* currentScene();
    GraphScene const* currentScene() const;

    /**
     * @brief Creates a new scene for the given graph. Fails if a scene is
     * already registered for the given graph. If the scene is the only one
     * registered it is also set as the current scene.
     * @param graph Graph to create a scene for.
     * @return
     */
    GraphScene* createScene(Graph& graph);

public slots:

    /**
     * @brief Opens the graph in a new scene. The scene is created if it does
     * not exist already.
     * @param graph Graph to open
     */
    void openGraph(Graph* graph);

    /**
     * @brief Opens the graph referenced by the given uuid in a new scene.
     * The scene is created if it does not exist already.
     * @param graph Graph to open
     */
    void openGraphByUuid(QString const& graphUuid);

private slots:

    /**
     * @brief Updates scene mamager if a scene was deleted
     */
    void onSceneRemoved();

private:

    QPointer<GraphView> m_view;

    struct Entry
    {
        unique_qptr<GraphScene, DeferredDeleter> scene;
        bool markedForDeletion = false; // indicator if graph is about to be deleted
    };

    std::vector<Entry> m_scenes;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENEMANAGER_H
