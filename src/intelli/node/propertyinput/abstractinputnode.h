/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_ABSTRACTINPUTNODE_H
#define GT_INTELLI_ABSTRACTINPUTNODE_H

#include <intelli/node.h>

#include <memory>

#include <gt_boolproperty.h>

namespace intelli
{
class GT_INTELLI_EXPORT AbstractInputNode : public intelli::Node
{
public:
    GtAbstractProperty& getProperty()
    {
        return *m_value;
    }

protected:
    explicit AbstractInputNode(
            QString const& nodeId,
            std::unique_ptr<GtAbstractProperty>&& pointer) :
        intelli::Node(nodeId),
        m_value(std::move(pointer)),
        m_isArg(QStringLiteral("isArg"), tr("isArg"), tr("isArg"))
    {
        registerProperty(*m_value);
        registerProperty(m_isArg);
    };

    /// unique pointer
    std::unique_ptr<GtAbstractProperty> m_value;

    GtBoolProperty m_isArg;
};

}
#endif // GT_INTELLI_ABSTRACTINPUTNODE_H
