/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTINPUTNODE_H
#define GT_INTELLI_OBJECTINPUTNODE_H

#include <intelli/node.h>

#include <gt_objectlinkproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT ObjectInputNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE ObjectInputNode();

    /**
     * @brief linkedObject
     * @return
     */
    GtObject* linkedObject(GtObject* root = nullptr);
    GtObject const* linkedObject(GtObject const* root = nullptr) const;

    void setValue(QString const& uuid);

protected:

    void eval() override;

private:

    GtObjectLinkProperty m_object;

    /// member to keep track of last object pointer to disconnect changed signals
    QPointer<GtObject> m_lastObject;

    PortId m_out;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTINPUTNODE_H
