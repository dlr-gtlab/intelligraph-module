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

class GT_INTELLI_EXPORT StringData final : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE StringData(QString val = {});

    Q_INVOKABLE QString value() const;

private:
    QString m_data;
};

class StringListData;

} // namespace intelli

GT_INTELLI_DECLARE_LIST_TYPE(intelli::StringData, intelli::StringListData);

#endif // GT_INTELLI_STRINGDATA_H
