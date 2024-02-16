/* GTlab - Gas Turbine laboratory
 * copyright 2009-2024 by DLR
 *
 *  Created on: 16.2.2024
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef TESTNODEDATA_H
#define TESTNODEDATA_H

#include <intelli/nodedata.h>


class TestNodeData : public intelli::NodeData
{
    Q_OBJECT

public:

    static void registerOnce();

    Q_INVOKABLE TestNodeData(double value = {});

    /// getter for value
    Q_INVOKABLE double myDouble() const;

    /// getter to test argument forwarding
    Q_INVOKABLE double myDoubleModified(int i, QString const& s) const;

    /// setter
    Q_INVOKABLE void setMyDouble(double value);

private:

    double m_value;
};

#endif // TESTNODEDATA_H
