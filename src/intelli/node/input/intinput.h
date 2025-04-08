/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_INTINPUTNODE_H
#define GT_INTELLI_INTINPUTNODE_H

#include <intelli/node.h>
#include <intelli/property/metaenum.h>

#include <gt_intproperty.h>
#include <gt_boolproperty.h>

#include "intelli/private/utils.h"

namespace intelli
{

class GT_INTELLI_EXPORT IntInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE IntInputNode();

    /// Getter for the current value, may not be within lower and upper bounds
    /// if not using bounds.
    int value() const;
    /// Setter for current value, will be bound to min/max when using bounds.
    void setValue(int value);

    int lowerBound() const;
    void setLowerBound(int value);

    int upperBound() const;
    void setUpperBound(int value);

    bool useBounds() const;
    void setUseBounds(bool value);

protected:

    void eval() override;

private:

    /// Current value
    GtIntProperty m_value;

    /// Upper bound
    GtIntProperty m_min;

    /// Lower bound
    GtIntProperty m_max;

    /// Whether bounds (min, max) should be enforced. Dependent on input type.
    GtBoolProperty m_useBounds;

    /// Holds input mode, used to remember state of GUI.
    MetaEnumProperty m_inputMode;

    GtBoolProperty m_joystick;

    PortId m_out;

    utils::JoystickReader* m_joyStickObj;
};

} // namespace intelli

#endif // GT_INTELLI_INTINPUTNODE_H
