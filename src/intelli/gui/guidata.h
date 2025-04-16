/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GUIDATA_H
#define GT_INTELLI_GUIDATA_H

#include <intelli/globals.h>

#include <gt_state.h>
#include <gt_object.h>
#include <gt_propertystructcontainer.h>

namespace intelli
{

class Graph;
class LocalStateContainer;

class GuiData : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GuiData(GtObject* parent = nullptr);

    static LocalStateContainer* accessLocalStates(Graph& graph);
};

class LocalStateContainer : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE LocalStateContainer(GtObject* parent = nullptr);

    void init();

    void setNodeCollapsed(NodeUuid const& nodeUuid, bool collapsed = true);

    bool isNodeCollapsed(NodeUuid const& nodeUuid) const;

signals:

    void nodeCollapsedChanged(QString const& nodeUuid, bool isCollapsed);

private:

    GtPropertyStructContainer m_collapsed;

    QStringList m_collapsedMap;
};

} // namespace intelli

#endif // GT_INTELLI_GUIDATA_H
