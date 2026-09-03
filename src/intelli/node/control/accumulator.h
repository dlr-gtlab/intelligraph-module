/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#ifndef GT_INTELLI_ACCUMULATORGRAPHNODE_H
#define GT_INTELLI_ACCUMULATORGRAPHNODE_H

#include <intelli/graph.h>
#include <intelli/node/groupinputprovider.h>
#include <intelli/node/groupoutputprovider.h>

namespace intelli
{

class LastIterationProvider : public GraphInputProvider
{
    Q_OBJECT

public:

    Q_INVOKABLE LastIterationProvider() {}
};

class AccumulatorGraphNode : public Graph
{
    Q_OBJECT

public:

    Q_INVOKABLE AccumulatorGraphNode();

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders() final {}

protected:

    using Graph::inputProvider;
    using Graph::inputNode;
    using Graph::outputProvider;
    using Graph::outputNode;

    void eval() override;

private:

    PortId m_listIn, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_ACCUMULATORGRAPHNODE_H
