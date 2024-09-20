/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_CONNECTIONGROUP_H
#define GT_INTELLI_CONNECTIONGROUP_H

#include "gt_objectgroup.h"

namespace intelli
{

class ConnectionGroup : public GtObjectGroup
{
    Q_OBJECT

public:

    Q_INVOKABLE ConnectionGroup(GtObject* parent = nullptr);

signals:

    void mergeConnections();

protected:

    // keep graph model up date if a connection was restored
    void onObjectDataMerged() override;
};

} // namespace intelli

#endif // GT_INTELLI_CONNECTIONGROUP_H
