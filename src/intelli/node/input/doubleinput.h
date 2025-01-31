/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_DOUBLEINPUTNODE_H
#define GT_INTELLI_DOUBLEINPUTNODE_H

#include <intelli/node.h>
#include <intelli/property/metaenum.h>

#include <gt_doubleproperty.h>
#include <gt_boolproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT DoubleInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE DoubleInputNode();

    /// Getter for the current value, may not be within lower and upper bounds
    /// if not using bounds.
    double value() const;
    /// Setter for current value, will be bound to min/max when using bounds.
    void setValue(double value);

    double lowerBound() const;
    void setLowerBound(double value);

    double upperBound() const;
    void setUpperBound(double value);

    bool useBounds() const;
    void setUseBounds(bool value);

protected:

    void eval() override;

private:

    /// Current value
    GtDoubleProperty m_value;

    /// Upper bound
    GtDoubleProperty m_min;

    /// Lower bound
    GtDoubleProperty m_max;

    /// Whether bounds (min, max) should be enforced. Dependent on input type.
    GtBoolProperty m_useBounds;

    /// Holds input mode, used to remember state of GUI.
    MetaEnumProperty m_inputMode;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_DOUBLEINPUTNODE_H
