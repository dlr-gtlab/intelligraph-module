/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 26.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef NPDOUBLEINPUTNODE_H
#define NPDOUBLEINPUTNODE_H

#include <gt_modeproperty.h>
#include <gt_modetypeproperty.h>
#include <gt_doubleproperty.h>

#include "abstractinputnode.h"

namespace intelli
{
class GT_INTELLI_EXPORT DoubleInputNode : public AbstractInputNode
{
    Q_OBJECT
public:
    Q_INVOKABLE DoubleInputNode();

    double value() const;

    void setValue(double value);

    void eval() override;

private slots:
    void onWidgetValueChanges(double newVal);

signals:
    void triggerWidgetUpdate(double val, double min, double max);

    void displayModeChanged(QString const& type);

private:
    GtDoubleProperty m_min;

    GtDoubleProperty m_max;

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
#endif // NPDOUBLEINPUTNODE_H
