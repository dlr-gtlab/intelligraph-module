/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/icons.h"

QIcon const&
gt::gui::icon::intelli::intelliGraph()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/intelligraph.svg")
    );
    return icon;
}

QIcon const&
gt::gui::icon::intelli::node()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/node.svg")
    );
    return icon;
}

QIcon const&
gt::gui::icon::intelli::textAlignLeft()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/format-align-left.svg")
    );
    return icon;
}

QIcon const&
gt::gui::icon::intelli::textAlignCenter()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/format-align-center.svg")
    );
    return icon;
}

QIcon const&
gt::gui::icon::intelli::textAlignRight()
{
    static QIcon icon = gt::gui::getIcon(
        QStringLiteral(":/intelligraph-icons/format-align-right.svg")
    );
    return icon;
}
