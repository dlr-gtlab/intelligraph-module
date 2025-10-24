/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/utilities.h>

#include <QWidget>
#include <QVBoxLayout>

std::unique_ptr<QWidget>
intelli::utils::makeWidgetWithLayout()
{
    auto base = std::make_unique<QWidget>();
    auto* layout = new QVBoxLayout(base.get());
    layout->setContentsMargins(0, 0, 0, 0);
    return base;
}
