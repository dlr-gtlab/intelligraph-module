#ifndef GT_IGQWTBARCHARTNODE_H
#define GT_IGQWTBARCHARTNODE_H

#include "gt_igabstractqwtnode.h"

class GtIgQwtBarChartNode : public GtIgAbstractQwtNode
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgQwtBarChartNode();

protected:

    void initWidget() override;
};

#endif // GT_IGQWTBARCHARTNODE_H
