/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 07.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */

#include <intelli/data/bytearray.h>

using namespace intelli;

ByteArrayData::ByteArrayData(QByteArray ba) :
    NodeData("byte_data"),
    m_data(std::move(ba))
{}

QByteArray
ByteArrayData::value() const
{
    return m_data;
}
