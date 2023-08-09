/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email:
 */

#ifndef NDSNODEEDITOR_H
#define NDSNODEEDITOR_H

#include "intelli/graph.h"
#include "intelli/memory.h"
#include "intelli/gui/graphscene.h"

#include "gt_mdiitem.h"
#include "gt_utilities.h"

class QMenu;

namespace intelli
{

class GraphView;

/**
 * @generated 1.2.0
 * @brief The GraphEditor class
 */
class GraphEditor : public GtMdiItem
{
    Q_OBJECT

    struct Cleanup
    {
        void operator()() { if (data) data->clearModelAdapter(); }
        QPointer<Graph> data;
    };

public:

    Q_INVOKABLE GraphEditor();

    void setData(GtObject* obj) override;

private:

    gt::Finally<Cleanup> m_cleanup{Cleanup{}};
    /// scene
    volatile_ptr<GraphScene> m_scene = nullptr;
    /// view
    GraphView* m_view = nullptr;
};

} // namespace intelli

#endif // NDSNODEEDITOR_H
