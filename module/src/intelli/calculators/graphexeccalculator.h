/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 06.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef GRAPHEXECCALCULATOR_H
#define GRAPHEXECCALCULATOR_H

#include <gt_calculator.h>
#include <gt_objectlinkproperty.h>
#include <gt_propertystructcontainer.h>

namespace intelli
{

class GraphExecCalculator : public GtCalculator
{
    Q_OBJECT
public:
    Q_INVOKABLE GraphExecCalculator();

    bool run() override;

private:
    /// component to read
    GtObjectLinkProperty m_intelli;

    GtPropertyStructContainer m_numberNodeContainer;
};
}
#endif // GRAPHEXECCALCULATOR_H
