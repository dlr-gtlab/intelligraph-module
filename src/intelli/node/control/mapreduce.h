
#ifndef GT_INTELLI_MAPREDUCEGROUPNODE_H
#define GT_INTELLI_MAPREDUCEGROUPNODE_H

#include <intelli/graph.h>

#include <gt_enumproperty.h>

namespace intelli
{

class MapReduceGroupNode : public Graph
{
    Q_OBJECT

public:

    Q_INVOKABLE MapReduceGroupNode();

    /**
     * @brief initializes the input and output of this graph
     */
    void initInputOutputProviders() final {}

    enum ReduceOperation
    {
        ReduceAdd,
        ReduceSubstract,
        ReduceMultiply,
        ReduceMax,
        ReduceMin
    };
    Q_ENUM(ReduceOperation)

protected:

    using Graph::inputProvider;
    using Graph::inputNode;
    using Graph::outputProvider;
    using Graph::outputNode;

    void eval() override;

private:

    PortId m_listIn, m_out, m_index;

    GtEnumProperty<ReduceOperation> m_operation;
};

} // namespace intelli

#endif // MAPREDUCEGROUPNODE_H
