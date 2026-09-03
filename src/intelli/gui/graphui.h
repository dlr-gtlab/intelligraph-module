/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUI_H
#define GT_INTELLI_GRAPHUI_H

#include <intelli/gui/nodeui.h>

namespace intelli
{

class GraphUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphUI(Options options = NoOption);

    QIcon displayIcon(Node const& node) const override;

    QStringList openWith(GtObject* obj) override;

protected:

    enum GraphNodeAction
    {
        ProviderNodeAction = UserNodeAction << 0,
        UserNodeAction = ProviderNodeAction << 1
    };

    enum GraphPortAction
    {
        ProviderPortAction = UserPortAction << 0,
        UserPortAction = ProviderPortAction << 1
    };

    NodeActionList defaultNodeActions() const override;

    PortActionList defaultPortActions() const override;

    /**
     * @brief Clears the intelli graph (i.e. removes all nodes and connections)
     * @param obj Intelli graph to clear
     */
    static void clearGraphNode(GtObject* obj);

    static void duplicateGraph(GtObject* obj);

    /**
     * @brief Prompts the user and adds an input port to the given dynamic node
     * @param obj
     */
    static void addInputProviderPort(GtObject* obj);

    /**
     * @brief Prompts the user and adds an output port to the given dynamic node
     * @param obj
     */
    static void addOutputProviderPort(GtObject* obj);

    /**
     * @brief Prompts the user to edit the given dynamic port
     * @param obj
     * @param type
     * @param idx
     */
    static void editProviderPort(Node* obj, PortType type, PortIndex idx);

    /**
     * @brief Deletes a dynamic port
     * @param obj
     * @param type
     * @param idx
     */
    static void deleteProviderPort(Node* obj, PortType type, PortIndex idx);
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUI_H
