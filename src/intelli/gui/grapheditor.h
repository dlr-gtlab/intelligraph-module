/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHEDITOR_H
#define GT_INTELLI_GRAPHEDITOR_H

#include <intelli/memory.h>

#include <gt_mdiitem.h>

#include <QPointer>

namespace intelli
{

class GraphScene;
class GraphSceneManager;
class GraphView;
class GraphViewOverlay;

/**
 * @generated 1.2.0
 * @brief The GraphEditor class
 */
class GraphEditor : public GtMdiItem
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphEditor();
    ~GraphEditor();

    void setData(GtObject* obj) override;

protected:

    void initialized() override;

private:

    /// scene manager
    GraphSceneManager* m_sceneManager = nullptr;
    /// view
    GraphView* m_view = nullptr;
    /// overlay
    GraphViewOverlay* m_overlay = nullptr;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHEDITOR_H
