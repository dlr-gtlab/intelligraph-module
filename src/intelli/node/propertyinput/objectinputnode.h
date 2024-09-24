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

#include <gt_objectlinkproperty.h>
#include "abstractinputnode.h"

namespace intelli
{
class GT_INTELLI_EXPORT ObjectInputNode : public AbstractInputNode
{
    Q_OBJECT
public:
    Q_INVOKABLE ObjectInputNode();

    /**
     * @brief linkedObject
     * @return
     */
    GtObject* linkedObject(GtObject* root = nullptr);
    GtObject const* linkedObject(GtObject* root = nullptr) const;

    void setValue(QString const& uuid);

    void eval() override;

private:
    PortId m_out;

    GtObjectLinkProperty* objLinkProp();

    const GtObjectLinkProperty* objLinkProp() const;

    /// member to keep track of last object pointer to disconnect changed signals
    QPointer<GtObject> m_lastObject;

    void revertProperty();
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTINPUTNODE_H
