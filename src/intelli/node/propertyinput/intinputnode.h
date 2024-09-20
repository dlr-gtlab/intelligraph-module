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
#include <gt_modeproperty.h>
#include <gt_modetypeproperty.h>

#include <gt_intproperty.h>
#include "abstractinputnode.h"

namespace intelli
{
class GT_INTELLI_EXPORT IntInputNode : public AbstractInputNode
{
    Q_OBJECT
public:
    Q_INVOKABLE IntInputNode();

    int value() const;

    void setValue(int value);

    void eval() override;

private slots:
    void onWidgetValueChanges(int newVal);

signals:
    void triggerWidgetUpdate(double val, double min, double max);

    void displayModeChanged(QString const& type);

private:
    GtIntProperty m_min;

    GtIntProperty m_max;

    GtModeProperty m_displayType;

    GtModeTypeProperty m_textDisplay;

    GtModeTypeProperty m_dial;

    GtModeTypeProperty m_sliderH;

    GtModeTypeProperty m_sliderV;

    PortId m_out;

    QMetaObject::Connection m_minPropConnection;

    QMetaObject::Connection m_maxPropConnection;
};

} // namespace intelli

#endif // GT_INTELLI_INTINPUTNODE_H
