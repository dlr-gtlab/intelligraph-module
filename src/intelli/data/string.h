/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_STRINGDATA_H
#define GT_INTELLI_STRINGDATA_H

#include <intelli/nodedata.h>

namespace intelli
{

class GT_INTELLI_EXPORT StringData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE StringData(QString val = {});

    Q_INVOKABLE QString value() const;

private:
    QString m_data;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGDATA_H
