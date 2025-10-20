/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUSERVARIABLESINPUTNODE_H
#define GT_INTELLI_GRAPHUSERVARIABLESINPUTNODE_H

#include <intelli/dynamicnode.h>

#include <QPointer>

namespace intelli
{

class GraphUserVariables;
class GraphUserVariablesInputNode : public DynamicNode
{
    Q_OBJECT

public:
    
    Q_INVOKABLE GraphUserVariablesInputNode();
    ~GraphUserVariablesInputNode();

protected:

    void nodeEvent(NodeEvent const* e) override;

    void eval() override;

    void onObjectDataMerged() override;

private:

    QPointer<GraphUserVariables const> m_uv;

    void updatePorts();
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUSERVARIABLESINPUTNODE_H
