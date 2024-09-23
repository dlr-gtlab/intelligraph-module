/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLDATA_H
#define GT_INTELLI_BOOLDATA_H

#include <intelli/nodedata.h>

namespace intelli
{

class GT_INTELLI_EXPORT BoolData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE BoolData(bool val = {});

    Q_INVOKABLE bool value() const;

private:
    bool m_data;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLDATA_H
