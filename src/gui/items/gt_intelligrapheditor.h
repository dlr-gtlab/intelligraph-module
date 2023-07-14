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

#include "gt_intelligraphnode.h"

#include <QtNodes/Definitions>
#include <QtNodes/DataFlowGraphModel>
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/GraphicsView>

class QMenu;
class GtIntelliGraph;

/**
 * @generated 1.2.0
 * @brief The NdsNodeEditor class
 */
class GtIntelliGraphEditor : public GtMdiItem
{
    Q_OBJECT

public:

    using QtNodeId       = QtNodes::NodeId;

    Q_INVOKABLE GtIntelliGraphEditor();
    ~GtIntelliGraphEditor();

    void setData(GtObject* obj) override;

private:

    QPointer<GtIntelliGraph> m_data = nullptr;

    /// model
    QPointer<QtNodes::DataFlowGraphModel> m_model = nullptr;
    /// scene
    QtNodes::DataFlowGraphicsScene* m_scene = nullptr;
    /// view
    QtNodes::GraphicsView* m_view = nullptr;

    QMenu* m_sceneMenu = nullptr;

private slots:

    void loadFromJson();

    void saveToJson();

    void onNodePositionChanged(QtNodeId nodeId);

    void onNodeSelected(QtNodeId nodeId);

    void onNodeDoubleClicked(QtNodeId nodeId);

    void onNodeContextMenu(QtNodeId nodeId, QPointF pos);

private:

    void loadScene(QJsonObject const& scene);

    void makeGroupNode(std::vector<QtNodeId> const& selectedNodeIds);
};

#endif // NDSNODEEDITOR_H
