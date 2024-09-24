/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodedata.h"

using namespace intelli;

QString const&
NodeData::typeName() const
{
    return m_typeName;
}

QString
NodeData::typeId() const
{
    return metaObject()->className();
}

NodeData::NodeData(QString typeName) :
    m_typeName(std::move(typeName))
{

}
