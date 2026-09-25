
#include "intelli/node/number/numberaccumulatornode.h"

#include "intelli/data/list.h"
#include "intelli/data/double.h"

using namespace intelli;

NumberAccumulatorNode::NumberAccumulatorNode() :
    Node("Number Accumulator")
{
    // in ports
    m_in = addInPort(makePort(listTypeId<DoubleData>()).setCaption("list"));
    m_out = addOutPort(makePort(typeId<DoubleData>()).setCaption("result"));
}

void
NumberAccumulatorNode::eval()
{
    NodeDataPtr nodeData = this->nodeData(m_in);
    if (!nodeData) return;

    auto listData = qobject_cast<BaseListData const*>(nodeData.get());
    if (!listData) return evalFailed();

    double result = 0;
    for (NodeDataPtr const& current : listData->iterate())
    {
        if (Ptr<DoubleData> converted = convert<DoubleData>(current))
        {
            result += converted->value();
        }
    }

    setNodeData(m_out, makeNodeData<DoubleData>(result));
}

