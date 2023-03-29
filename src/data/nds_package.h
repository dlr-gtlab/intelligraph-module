/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef NDSPACKAGE_H
#define NDSPACKAGE_H

#include "gt_package.h"

/**
 * @generated 1.2.0
 * @brief The NdsPackage class
 */
class NdsPackage : public GtPackage
{
    Q_OBJECT

public:

    Q_INVOKABLE NdsPackage();
};

#endif // NDSPACKAGE_H
