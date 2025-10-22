/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_BOOLNODEUI_H
#define GT_INTELLI_BOOLNODEUI_H

#include <intelli/gui/nodeui.h>

namespace intelli
{

class BoolNodeUI : public NodeUI
{
    Q_OBJECT

public:
    
    Q_INVOKABLE BoolNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_BOOLNODEUI_H
