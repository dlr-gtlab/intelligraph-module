#ifndef GT_INTELLI_BINARYDISPLAY_H
#define GT_INTELLI_BINARYDISPLAY_H

#include <intelli/dynamicnode.h>

namespace intelli
{

class BinaryDisplayNode : public DynamicNode
{
    Q_OBJECT

public:

    Q_INVOKABLE BinaryDisplayNode();

    unsigned inputValue();
};

} // namespace intelli

#endif // GT_INTELLI_BINARYDISPLAY_H
