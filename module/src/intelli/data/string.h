/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 22.01.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef GT_INTELLI_STRINGDATA_H
#define GT_INTELLI_STRINGDATA_H
#include "intelli/nodedata.h"

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
