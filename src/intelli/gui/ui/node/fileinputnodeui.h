/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_FILEINPUTNODEUI_H
#define GT_INTELLI_FILEINPUTNODEUI_H

#include <intelli/gui/nodeui.h>

namespace intelli
{

class FileInputNodeUI : public NodeUI
{
    Q_OBJECT

public:

    Q_INVOKABLE FileInputNodeUI();

    WidgetFactoryFunction centralWidgetFactory(Node const& node) const override;
};

} // namespace intelli

#endif // GT_INTELLI_FILEINPUTNODEUI_H
