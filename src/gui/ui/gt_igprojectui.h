/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

#ifndef GT_IGPROJECTUI_H
#define GT_IGPROJECTUI_H

#include "gt_objectui.h"

/**
 * @generated 1.2.0
 * @brief The GtIgProjectUI class
 */
class GtIgProjectUI : public GtObjectUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgProjectUI();

public slots:

    void openNodeEditor(GtObject* obj);
};

#endif // GT_IGPROJECTUI_H
