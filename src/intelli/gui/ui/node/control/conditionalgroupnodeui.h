/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
 */

#ifndef GT_INTELLI_CONDITIONALGROUPNODEUI_H
#define GT_INTELLI_CONDITIONALGROUPNODEUI_H

#include <intelli/gui/graphui.h>

namespace intelli
{

class ConditionalGroupNodeUI : public GraphUI
{
    Q_OBJECT

public:

    Q_INVOKABLE ConditionalGroupNodeUI();

    QIcon displayIcon(Node const& node) const override;

private:

    /**
     * @brief Adds an input port to a dynamic node
     * @param obj
     */
    static void addInPort(GtObject* obj);

    /**
     * @brief Adds an output port to a dynamic node
     * @param obj
     */
    static void addOutPort(GtObject* obj);

    static void editPort(Node* obj, PortType type, PortIndex idx);

    static void deletePort(Node* obj, PortType type, PortIndex idx);
};

} // namespace intelli

#endif // GT_INTELLI_CONDITIONALGROUPNODEUI_H
