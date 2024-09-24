/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BYTEARRAY_H
#define GT_INTELLI_BYTEARRAY_H

#include <intelli/nodedata.h>

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

#endif // GT_INTELLI_BYTEARRAY_H
