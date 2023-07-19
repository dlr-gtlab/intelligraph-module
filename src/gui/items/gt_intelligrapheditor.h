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
#include "gt_intelligraphscene.h"
#include "gt_igvolatileptr.h"

#include "gt_mdiitem.h"
#include "gt_utilities.h"

class QMenu;
class GtIntelliGraphView;

/**
 * @generated 1.2.0
 * @brief The NdsNodeEditor class
 */
class GtIntelliGraphEditor : public GtMdiItem
{
    Q_OBJECT

    struct Cleanup
    {
        void operator()() { if (data) data->clearModelAdapter(); }
        QPointer<GtIntelliGraph> data;
    };

public:

    Q_INVOKABLE GtIntelliGraphEditor();

    void setData(GtObject* obj) override;

private:

    gt::Finally<Cleanup> m_cleanup{Cleanup{}};
    /// scene
    gt::ig::volatile_ptr<GtIntelliGraphScene> m_scene = nullptr;
    /// view
    GtIntelliGraphView* m_view = nullptr;
};

#endif // NDSNODEEDITOR_H
