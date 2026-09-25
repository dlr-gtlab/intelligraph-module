
#ifndef GT_INTELLI_TOLISTNODE_H
#define GT_INTELLI_TOLISTNODE_H

#include <intelli/dynamicnode.h>

#include <intelli/property/stringselection.h>

namespace intelli
{

class ToListNode : public DynamicNode
{
    Q_OBJECT
public:

    Q_INVOKABLE ToListNode();

protected:

    void eval() override;

    PortId insertPort(PortOption option, PortType type, PortInfo port, int idx = -1) override;

private:

    PortId m_out;
    StringSelectionProperty m_selectedTypeId;
};

} // namespace intelli

#endif // GT_INTELLI_TOLISTNODE_H
