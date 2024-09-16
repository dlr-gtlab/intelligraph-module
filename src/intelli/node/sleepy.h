/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_SLEEPYNODE_H
#define GT_INTELLI_SLEEPYNODE_H

#include <intelli/node.h>

#include <gt_intproperty.h>

namespace intelli
{

class SleepyNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE SleepyNode();

signals:

    void timePassed(int progress);

protected:

    void eval() override;

private:

    GtIntProperty m_timer;

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_SLEEPYNODE_H
