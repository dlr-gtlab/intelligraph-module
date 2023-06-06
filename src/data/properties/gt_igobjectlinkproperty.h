/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 13.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_IGOBJECTLINKPROPERTY_H
#define GT_IGOBJECTLINKPROPERTY_H

#include "gt_objectlinkproperty.h"
#include "gt_intelligraph_exports.h"

class GT_IG_EXPORT GtIgObjectLinkProperty : public GtObjectLinkProperty
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

#endif // GT_IGOBJECTLINKPROPERTY_H
