/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_OBJECTLINKPROPERTY_H
#define GT_INTELLI_OBJECTLINKPROPERTY_H

#include <intelli/exports.h>

#include <gt_objectlinkproperty.h>

namespace intelli
{

class GT_INTELLI_EXPORT ObjectLinkProperty : public GtObjectLinkProperty
{
    Q_OBJECT

public:

    using GtObjectLinkProperty::GtObjectLinkProperty;
    using GtObjectLinkProperty::operator QString;
    using GtObjectLinkProperty::operator();

    void setAllowedClasses(QStringList const& filter)
    {
        m_allowedClasses = filter;
    }
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTLINKPROPERTY_H
