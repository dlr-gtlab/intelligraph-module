/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/ui/guidataui.h>

#include <gt_icons.h>

using namespace intelli;

GuiDataUI::GuiDataUI() = default;

QIcon
GuiDataUI::icon(GtObject* obj) const
{
    return gt::gui::icon::data();
}
