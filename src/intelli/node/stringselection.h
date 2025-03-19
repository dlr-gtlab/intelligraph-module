/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#ifndef GT_INTELLI_STRINGSELECTION_H
#define GT_INTELLI_STRINGSELECTION_H

#include <intelli/node.h>

namespace intelli
{
class GT_INTELLI_EXPORT StringSelection : public Node
{
    Q_OBJECT
public:
    Q_INVOKABLE StringSelection();

protected:
    void eval() override;

private:
    /// ports for parent objet input and child object output
    PortId m_in, m_out;

    QString m_selection;
};
}

#endif // GT_INTELLI_STRINGSELECTION_H
