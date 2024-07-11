/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 07.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */

#ifndef GT_INTELLI_BYTEARRAYDATA_H
#define GT_INTELLI_BYTEARRAYDATA_H

#include "intelli/nodedata.h"

#include <QByteArray>

namespace intelli
{

class GT_INTELLI_EXPORT ByteArrayData : public NodeData
{
    Q_OBJECT

public:

    Q_INVOKABLE explicit ByteArrayData(QByteArray ba = {});

    QByteArray data() const { return m_data; }

    Q_INVOKABLE QByteArray value() const;

private:
    QByteArray m_data;
};

} // namespace intelli

#endif // GT_INTELLI_BYTEARRAYDATA_H
