/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef NDSNODEEDITOR_H
#define NDSNODEEDITOR_H

#include "gt_mdiitem.h"

#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>

using QtNodes::DataFlowGraphModel;
using QtNodes::DataFlowGraphicsScene;

/**
 * @generated 1.2.0
 * @brief The NdsNodeEditor class
 */
class NdsNodeEditor : public GtMdiItem
{
    Q_OBJECT

public:

    Q_INVOKABLE NdsNodeEditor();

    void setData(GtObject* obj) override;

    void showEvent() override;

private:
    DataFlowGraphModel m_graphModel;

    DataFlowGraphicsScene* m_scene;

private slots:
    void onSelectionChanged();

    void onNodeContextMenu();
};

#endif // NDSNODEEDITOR_H
