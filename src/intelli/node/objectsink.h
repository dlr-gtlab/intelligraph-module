/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTSINK_H
#define GT_INTELLI_OBJECTSINK_H

#include "gt_objectlinkproperty.h"
#include "intelli/node.h"

class QPushButton;

namespace intelli
{

class ObjectSink : public Node
{
    Q_OBJECT

public:
    Q_INVOKABLE ObjectSink();

private:
    PortId m_in;

    GtObjectLinkProperty m_target;

private slots:
    void doExport();
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTSINK_H
