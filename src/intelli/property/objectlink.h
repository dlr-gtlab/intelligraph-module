/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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
