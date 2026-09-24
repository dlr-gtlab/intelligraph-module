
#ifndef GT_INTELLI_NUMBERACCUMULATORNODE_H
#define GT_INTELLI_NUMBERACCUMULATORNODE_H

#include <intelli/node.h>

namespace intelli
{

class NumberAccumulatorNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE NumberAccumulatorNode();

protected:

    void eval() override;

private:

    PortId m_in, m_out;
};

} // namespace intelli

#endif // GT_INTELLI_NUMBERACCUMULATORNODE_H
