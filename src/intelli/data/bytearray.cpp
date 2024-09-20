/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/data/bytearray.h>

using namespace intelli;

ByteArrayData::ByteArrayData(QByteArray ba) :
    NodeData(QStringLiteral("byte_data")),
    m_data(std::move(ba))
{}

QByteArray
ByteArrayData::value() const
{
    return m_data;
}
