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

/**
 * @generated 1.2.0
 * @brief The NdsNodeEditor class
 */
class GtIntelliGraphEditor : public GtMdiItem
{
    Q_OBJECT

public:

    using QtNodeId       = QtNodes::NodeId;
    using QtConnectionId = QtNodes::ConnectionId;

    Q_INVOKABLE GtIntelliGraphEditor();
    ~GtIntelliGraphEditor();

    void setData(GtObject* obj) override;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;

private slots:

    void loadFromJson();

    void saveToJson();

    void onNodePositionChanged(QtNodeId nodeId);

    void onNodeSelected(QtNodeId nodeId);

private:

    void loadScene(QJsonObject const& scene);
};

#endif // NDSNODEEDITOR_H
