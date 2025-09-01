/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/nodeuidata.h"

#include <QIcon>

using namespace intelli;

struct NodeUIData::Impl
{
    QIcon icon;

    CustomDeleteFunction deleteFunction;
};

NodeUIData::NodeUIData() :
    pimpl(std::make_unique<Impl>())
{
    assert(pimpl);
}

NodeUIData::~NodeUIData() = default;

QIcon
NodeUIData::displayIcon() const
{
    return pimpl->icon;
}

void
NodeUIData::setDisplayIcon(QIcon icon)
{
    pimpl->icon = std::move(icon);
}

bool
NodeUIData::hasDisplayIcon() const
{
    return !pimpl->icon.isNull();
}

NodeUIData::CustomDeleteFunction const&
NodeUIData::customDeleteFunction() const
{
    return pimpl->deleteFunction;
}

bool
NodeUIData::hasCustomDeleteFunction() const
{
    return !!pimpl->deleteFunction;
}

void
NodeUIData::setCustomDeleteFunction(CustomDeleteFunction functor)
{
    pimpl->deleteFunction = std::move(functor);
}
