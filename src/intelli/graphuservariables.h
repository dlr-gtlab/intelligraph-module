/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHUSERVARIABLES_H
#define GT_INTELLI_GRAPHUSERVARIABLES_H

#include "intelli/exports.h"

#include <gt_platform.h>
#include <gt_object.h>

#include <memory.h>

namespace intelli
{

class GraphUserVariables : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE GraphUserVariables(GtObject* parent = nullptr);
    ~GraphUserVariables();

    Q_INVOKABLE bool setValue(QString const& key, QVariant const& value);

    Q_INVOKABLE bool remove(QString const& key);

    GT_NO_DISCARD GT_INTELLI_EXPORT Q_INVOKABLE
    bool hasValue(QString const& key) const;

    GT_NO_DISCARD GT_INTELLI_EXPORT Q_INVOKABLE
    QVariant value(QString const& key) const;

    GT_NO_DISCARD GT_INTELLI_EXPORT Q_INVOKABLE
    QStringList keys() const;

    GT_NO_DISCARD GT_INTELLI_EXPORT Q_INVOKABLE
    size_t size() const;

    GT_INTELLI_EXPORT
    void visit(std::function<void(QString const& key, QVariant const& value)> f) const;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUSERVARIABLES_H
