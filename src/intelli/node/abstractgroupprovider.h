/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_ABSTRACTGRAPHPROVIDER_H
#define GT_INTELLI_ABSTRACTGRAPHPROVIDER_H

#include <intelli/graph.h>
#include <intelli/node.h>

#include <gt_coreapplication.h>

namespace intelli
{

class AbstractGraphProvider : public Node
{
    PortType m_providerType;

    Q_OBJECT

public:

    inline PortType providerType() const { return m_providerType; }

    AbstractGraphProvider(PortType type, QString const& modelName) :
        Node(modelName),
        m_providerType(type)
    {
        assert(type != PortType::NoType && "Invalid PortType!");

        setFlag(UserDeletable, false);
        setNodeEvalMode(NodeEvalMode::Blocking);

        Position offset{250, 0};
        offset.rx() *=(type == PortType::In ? -1 : 1);
        setPos(pos() += offset);
    }

    PortId addPort(PortInfo port)
    {
        return insertPort(std::move(port));
    }

    PortId insertPort(PortInfo data, int idx = -1)
    {
        return invert(providerType()) == PortType::In ?
                   Node::insertInPort(std::move(data), idx) :
                   Node::insertOutPort(std::move(data), idx);
    }

    using Node::removePort;

protected:

    void eval() {}
};

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTGRAPHPROVIDER_H
