/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHEXECCALCULATOR_H
#define GT_INTELLI_GRAPHEXECCALCULATOR_H

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
#endif // GT_INTELLI_GRAPHEXECCALCULATOR_H
