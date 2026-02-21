/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#ifndef GT_INTELLI_STRINGSELECTIONNODE_H
#define GT_INTELLI_STRINGSELECTIONNODE_H

#include <intelli/node.h>
#include <gt_stringproperty.h>

namespace intelli
{

class StringSelectionNode : public Node
{
    Q_OBJECT
public:

    Q_INVOKABLE StringSelectionNode();

    QString selection() const;
    void setSelection(QString const& selection);
    QStringList options() const;

signals:

    void selectionChanged(QString const& selection);
    void optionsChanged(QStringList const& options);

protected:

    void eval() override;

private:

    /// ports for parent objet input and child object output
    PortId m_in, m_out;

    GtStringProperty m_selection;
};

} // namespace intelli

#endif // GT_INTELLI_STRINGSELECTIONNODE_H
