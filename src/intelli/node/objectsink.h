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

#include "intelli/node.h"

namespace intelli
{

class ObjectSink : public Node
{
    Q_OBJECT

public:
    Q_INVOKABLE ObjectSink();

protected:
    void eval() override;

private:
    PortId m_in;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTSINK_H
