/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_NODEUIDATA_H
#define GT_INTELLI_NODEUIDATA_H

#include <intelli/exports.h>

#include <memory>
#include <functional>

class QIcon;

namespace intelli
{

class Node;
class NodeUI;
class NodeUIData
{
    friend class NodeUI;

    NodeUIData();

public:

    using CustomDeleteFunction = std::function<bool (Node*)>;

    NodeUIData(NodeUIData const&) = delete;
    NodeUIData(NodeUIData&&) = delete;
    NodeUIData& operator=(NodeUIData const&) = delete;
    NodeUIData& operator=(NodeUIData&&) = delete;
    ~NodeUIData();

    GT_INTELLI_EXPORT QIcon displayIcon() const;

    GT_INTELLI_EXPORT void setDisplayIcon(QIcon icon);

    GT_INTELLI_EXPORT bool hasDisplayIcon() const;

    GT_INTELLI_EXPORT CustomDeleteFunction const& customDeleteFunction() const;

    GT_INTELLI_EXPORT bool hasCustomDeleteFunction() const;

    void setCustomDeleteFunction(CustomDeleteFunction functor);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_NODEUIDATA_H
