/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHSTATEMANAGER_H
#define GT_INTELLI_GRAPHSTATEMANAGER_H

#include <intelli/memory.h>

#include <QObject>
#include <QPointer>

class GtObject;

namespace intelli
{

class Graph;
class GraphView;
class GraphScene;

/**
 * @brief The GraphStateManager class. Handles the state creation for a graph
 * instance and its associated graph view. Can be used to save view, scene and
 * exec states persistently.
 */
class GraphStateManager : public QObject
{
    Q_OBJECT

public:

    GraphStateManager(Graph& graph, GraphView& view);
    ~GraphStateManager();

    /**
     * @brief Creates a state manager instance for view object. Ownership is
     * taken care of.
     * @param view View to create state manager for
     * @return Pointer to state manager.
     */
    static GraphStateManager* make(Graph& graph, GraphView& view);

    GtObject* guardianObject();

private slots:

    void onSceneChanged(GraphScene* scene);

private:

    QPointer<GraphView> m_view;

    unique_qptr<GtObject> m_guardian;

    /**
     * @brief Instantiates all global (=user specific) states. These states
     * are not shared with other users e.g. by exchanging the project files.
     * @param scene Scene
     */
    void setupUserStates(GraphScene* scene);

    /**
     * @brief Instantiates all states specific to a graph. These states are
     * stored within the project files
     * @param scene Scene
     */
    void setupLocalStates(GraphScene* scene);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSTATEMANAGER_H
