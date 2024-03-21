/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email:
 */

#ifndef NDSNODEEDITOR_H
#define NDSNODEEDITOR_H

#include "intelli/memory.h"
#include "intelli/gui/graphscene.h"

#include "gt_mdiitem.h"

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

public:

    Q_INVOKABLE GraphEditor();

    void setData(GtObject* obj) override;

private:

    /// scene
    volatile_ptr<GraphScene> m_scene = nullptr;
    /// view
    GraphView* m_view = nullptr;
};

} // namespace intelli

#endif // NDSNODEEDITOR_H
