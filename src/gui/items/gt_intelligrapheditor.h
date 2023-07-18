/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email:
 */

#ifndef NDSNODEEDITOR_H
#define NDSNODEEDITOR_H

#include "gt_intelligraph.h"
#include "gt_intelligraphview.h"
#include "gt_intelligraphscene.h"
#include "gt_igvolatileptr.h"

#include "gt_mdiitem.h"

#include <QtNodes/Definitions>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>

class QMenu;

/**
 * @generated 1.2.0
 * @brief The NdsNodeEditor class
 */
class GtIntelliGraphEditor : public GtMdiItem
{
    Q_OBJECT

    /// helper struct to close the graph model if its no longer used
    struct Cleanup
    {
        void operator()() { if (data) data->clearModelManager(); }

        QPointer<GtIntelliGraph> data;
    };

public:

    using QtNodeId       = QtNodes::NodeId;
    using QtPortType     = QtNodes::PortType;
    using QtPortIndex    = QtNodes::PortIndex;

    Q_INVOKABLE GtIntelliGraphEditor();

    void setData(GtObject* obj) override;

protected:

    QString customId(GtObject* obj) override;

private:

    gt::Finally<Cleanup> m_cleanup{Cleanup{}};
    /// scene
    gt::ig::volatile_ptr<GtIntelliGraphScene> m_scene = nullptr;
    /// view
    GtIntelliGraphView* m_view = nullptr;
};

#endif // NDSNODEEDITOR_H
