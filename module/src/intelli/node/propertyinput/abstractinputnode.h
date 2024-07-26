/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef ABSTRACTINPUTNODE_H
#define ABSTRACTINPUTNODE_H

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
        m_isArg("isArg", tr("isArg"), tr("isArg"))
    {
        registerProperty(*m_value);
        registerProperty(m_isArg);
    };

    /// unique pointer
    std::unique_ptr<GtAbstractProperty> m_value;

    GtBoolProperty m_isArg;
};

}
#endif // ABSTRACTINPUTNODE_H
