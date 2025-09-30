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

/**
 * @brief The GraphUserVariables class.
 * Storage class for per-graph global/static user variables. This can be thought
 * of as Environment Variables for graphs. Only one instance per graph hierarchy
 * (i.e. the root instance) should hold all variables.
 */
class GT_INTELLI_EXPORT GraphUserVariables : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE
    GraphUserVariables(GtObject* parent = nullptr);
    ~GraphUserVariables();

    /**
     * @brief Adds a new entry for `key` and sets its value to `value`. If
     * an entry for `key` already exsists it will be overwritten.
     * @param key Key
     * @param value Value
     * @return Success
     */
    Q_INVOKABLE
    bool setValue(QString const& key, QVariant const& value);

    /**
     * @brief Removes the entry for `key`.
     * @param key Key
     * @return Success. Returns false if key was not found.
     */
    Q_INVOKABLE
    bool remove(QString const& key);

    /**
     * @brief Removes all entries
     */
    void clear();

    /**
     * @brief Returns whether an entry for `key` exists.
     * @param key Key
     * @return Whether an entry for `key` exists.
     */
    GT_NO_DISCARD Q_INVOKABLE
    bool hasValue(QString const& key) const;

    /**
     * @brief Returns the value for the entry `key`.
     * @param key Key
     * @return Value (may be null if entry does not exist)
     */
    GT_NO_DISCARD Q_INVOKABLE
    QVariant value(QString const& key) const;

    /**
     * @brief Returns all keys of the entries as a stringlist.
     * @return Keys
     */
    GT_NO_DISCARD Q_INVOKABLE
    QStringList keys() const;

    /**
     * @brief Returns the number of entries.
     * @return Number of entries
     */
    GT_NO_DISCARD Q_INVOKABLE
    size_t size() const;

    /**
     * @brief Returns whether this instance has any entries
     * @return Whether this instance has any entries
     */
    GT_NO_DISCARD Q_INVOKABLE
    bool empty() const;

    /**
     * @brief Iterates over all entries and calls the function `f` with the
     * key and value as an argument
     * @param f Functor
     */
    void visit(std::function<void(QString const& key,
                                  QVariant const& value)> f) const;

    /**
     * @brief Merges all entries with `other`. The entries are copied from
     * `other` and then `other` is cleared.
     * @param other Other
     */
    void mergeWith(GraphUserVariables& other);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHUSERVARIABLES_H
