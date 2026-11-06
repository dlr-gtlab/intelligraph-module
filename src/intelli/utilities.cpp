/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/utilities.h"
#include <intelli/node/abstractgroupprovider.h>
#include <intelli/private/utils.h>

void
intelli::utils::synchronizePorts(Node& source,
                                 AbstractGraphProvider& target,
                                 std::function<NodePort(NodePort const&)> makePort)
{
    using namespace intelli;

    auto onPortInserted =
        [&source, &target, makePort](PortType type, PortIndex idx){

        if (type != target.providerType()) return;

        auto const makeError = [&source, type, idx](){
            return relativeNodePath(source) + QStringLiteral(": ") +
                   QObject::tr("Failed to add %3put port (%1/%2)!")
                       .arg(toString(idx),
                            toString(type),
                            type == PortType::In ? "in":"out");
        };

        PortInfo* srcPort = source.port(source.portId(type, idx));
        if (!srcPort)
        {
            gtError() << makeError() << QObject::tr("(Source port not found)");
            return;
        }

        NodePort const copy = makePort ? makePort(*srcPort) : *srcPort;

        PortId addedPortId = target.addPort(copy);
        if (!addedPortId.isValid())
        {
            gtError() << makeError()
                      << QObject::tr("(Adding port to provider failed)");
            return;
        }
        assert(addedPortId == srcPort->id());
    };

    auto onPortChanged =
        [&source, &target, makePort](PortId portId){

        assert(portId.isValid());

        PortType type = source.portType(portId);
        if (type != target.providerType()) return;

        auto const makeError = [&source, type, portId](){
            return relativeNodePath(source) + QStringLiteral(": ") +
                   QObject::tr("Failed to update %2put port (%1)!")
                       .arg(toString(portId),
                            type == PortType::In ? "in":"out");
        };

        PortInfo* srcPort = source.port(portId);
        if (!srcPort)
        {
            gtError() << makeError() << QObject::tr("(Source port not found)");
            return;
        }

        PortInfo* port = source.port(srcPort->id());
        if (!port)
        {
            gtError() << makeError()
                      << QObject::tr("(Updating port of provider failed)");
            return;
        }

        NodePort const copy = makePort ? makePort(*srcPort) : *srcPort;
        port->assign(copy);

        assert(port->id() == srcPort->id());

        emit target.portChanged(port->id());
    };

    auto onPortDeleted =
        [&source, &target](PortType type, PortIndex idx){

        if (type != target.providerType()) return;

        auto const makeError = [&source, type, idx](){
            return relativeNodePath(source) + QStringLiteral(": ") +
                   QObject::tr("Failed to delete %3put port (%1/%2)!")
                       .arg(toString(idx),
                            toString(type),
                            type == PortType::In ? "in":"out");
        };

        auto portId = source.portId(type, idx);
        if (!portId.isValid())
        {
            gtError() << makeError() << QObject::tr("(Source port not found)");
            return;
        }

        if (!target.removePort(portId))
        {
            gtError() << makeError()
                      << QObject::tr("(Removing port of provider failed)");
            return;
        }
    };

    QObject::connect(&source, &Node::portInserted,
                     &target, onPortInserted,
                     Qt::DirectConnection);
    QObject::connect(&source, &Node::portChanged,
                     &target, onPortChanged,
                     Qt::DirectConnection);
    QObject::connect(&source, &Node::portAboutToBeDeleted,
                     &target, onPortDeleted,
                     Qt::DirectConnection);
}
