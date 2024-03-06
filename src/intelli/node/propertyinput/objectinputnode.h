/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 28.02.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#ifndef OBJECTINPUTNODE_H
#define OBJECTINPUTNODE_H

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
#endif // NPOBJECTINPUTNODE_H
