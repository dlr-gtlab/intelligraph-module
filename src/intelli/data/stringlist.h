/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#ifndef GT_INTELLI_STRINGLISTDATA_H
#define GT_INTELLI_STRINGLISTDATA_H

#include <intelli/nodedata.h>

namespace intelli
{
class GT_INTELLI_EXPORT StringListData : public NodeData
{
    Q_OBJECT
public:
    Q_INVOKABLE StringListData(QStringList val = {});

    Q_INVOKABLE QString value() const;

    Q_INVOKABLE void setValue(const QStringList& val);

private:
    QStringList m_data;
};

} // namespace intelli
#endif // GT_INTELLI_STRINGLISTDATA_H
