/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 07.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef BYTEARRAYDATA_H
#define BYTEARRAYDATA_H

#include "intelli/nodedata.h"

#include <QByteArray>

namespace intelli
{

class GT_INTELLI_EXPORT ByteArrayData : public NodeData
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit ByteArrayData(const QByteArray& ds = {}) :
        NodeData("python var"),
        m_data(ds) {}

    QByteArray data() const { return m_data; }

private:
    QByteArray m_data;
};

} // namespace intelli
#endif // BYTEARRAYDATA_H
