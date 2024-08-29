/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef GT_INTELLI_GENERICCALCULATOREXECNODE_H
#define GT_INTELLI_GENERICCALCULATOREXECNODE_H

#include <intelli/node.h>
#include "gt_stringproperty.h"

class GtPropertyTreeView;

namespace intelli
{

class GenericCalculatorExecNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE GenericCalculatorExecNode();

    /**
     * @brief Static member function that allows other modules to append class
     * names of calculators to the internal whitelist. Only whitelisted
     * calculators can be executed.
     * @param whiteList Classnames of calculators to whitelist
     * @return success
     */
    static bool addToWhiteList(QStringList const& whiteList);

protected:

    void eval() override;

private slots:

    void updateCurrentObject();

    void onCurrentObjectDataChanged();

    void onPortConnected(PortId portId);

    void onPortDisconnected(PortId portId);

signals:

    /// signals that the current object/calculator changed
    void currentObjectChanged();

private:

    /// Helper struct to hide implementation details
    struct Impl;

    /// outport to indicate success of calculator execution
    PortId m_outSuccess;
    /// property to define the class name
    GtStringProperty m_className;
    /// dynamic input ports for the properties of the calculator
    QHash<PortId, QString> m_calcInPorts;
    /// dynamic output ports for the output data of the calculator
    QHash<PortId, QString> m_calcOutPorts;

    /// returns the current child objet
    GtObject* currentObject();

    /// init the input ports based on the properties
    void initPorts();

    /// clear the existing ports (e.g. if a new class type is selected)
    void clearPorts();
};

} // namespace intelli

#endif // GT_INTELLI_GENERICCALCULATOREXECNODE_H
