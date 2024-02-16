/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 22.01.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */

#include "intelli/data/string.h"

using namespace intelli;

StringData::StringData(QString val):
    NodeData("string"),
    m_data(std::move(val))
{

}

QString
StringData::value() const
{
    return m_data;
}
