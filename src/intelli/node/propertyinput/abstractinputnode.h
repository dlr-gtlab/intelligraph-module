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

class GT_INTELLI_EXPORT AbstractInputNode : public Node
{
public:

    GtAbstractProperty& getProperty()
    {
        return *m_value;
    }

protected:

    explicit AbstractInputNode(QString const& modelName,
                               std::unique_ptr<GtAbstractProperty> pointer) :
        Node(modelName),
        m_value(std::move(pointer))
    {
        registerProperty(*m_value);
    };

    /// unique pointer
    std::unique_ptr<GtAbstractProperty> m_value;
};

} // namespace intelli

#endif // GT_INTELLI_ABSTRACTINPUTNODE_H
