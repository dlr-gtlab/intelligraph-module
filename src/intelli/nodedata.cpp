/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/nodedata.h"
#include "intelli/nodedatafactory.h"

#include <gt_logging.h>

using namespace intelli;

QString const&
NodeData::typeName() const noexcept
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
{ }

void
NodeData::logInvokeError(QByteArray const& rtypeName, QString const& methodName)
{
    gtTrace()
        << tr("Invoking member function '%1 %2(...)' failed!")
               .arg(rtypeName, methodName);
}

NodeDataPtr
intelli::convert(NodeDataPtr const& data, TypeId const& to)
{
    // forwarding call
    return NodeDataFactory::instance().convert(data, to);
}
