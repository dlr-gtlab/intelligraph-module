/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef GT_INTELLI_PACKAGE_H
#define GT_INTELLI_PACKAGE_H

#include <intelli/exports.h>

#include <gt_package.h>

namespace intelli
{

/**
 * @generated 1.2.0
 * @brief The GtIgPackage class
 */
class Package : public GtPackage
{
    Q_OBJECT

public:

    Q_INVOKABLE Package();
};

} // namespace intelli

#endif // GT_INTELLI_PACKAGE_H