/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef NDSPROJECTUI_H
#define NDSPROJECTUI_H

#include "gt_objectui.h"

/**
 * @generated 1.2.0
 * @brief The NdsProjectUI class
 */
class NdsProjectUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE NdsProjectUI();

public slots:

    void openNodeEditor(GtObject* obj);
};

#endif // NDSPROJECTUI_H
