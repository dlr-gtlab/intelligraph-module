/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_PORTEDITDIALOG_H
#define GT_INTELLI_PORTEDITDIALOG_H

#include <intelli/exports.h>
#include <intelli/globals.h>

#include <QDialog>

#include <memory>

namespace intelli
{

class GT_INTELLI_EXPORT PortEditDialog : public QDialog
{
    Q_OBJECT

public:

    enum Option : uint8_t
    {
        NoOption = 0,
        /// Hides list types and allows the use to toggle the list types using
        /// a button instead
        AllowListTypes
    };

    explicit PortEditDialog(PortType portType,
                            Option option = NoOption,
                            QStringList const& typeWhiteList = {});

    ~PortEditDialog();

    /**
     * @brief Applies port's type-id
     * @param typeId
     */
    void setTypeId(TypeId const& typeId);

    /**
     * @brief Applies port's caption
     * @param caption
     */
    void setCaption(QString const& caption);

    /**
     * @brief Applies port's caption visible flag
     * @param visible
     */
    void setCaptionVisible(bool visible = true);

    /**
     * @brief Applies port's optional flag
     * @param optional
     */
    void setOptional(bool optional = true);

    /**
     * @brief Returns the port's type-id
     * @return Type-id of the port
     */
    TypeId typeId() const;

    /**
     * @brief Returns the port's caption
     * @return Port caption
     */
    QString caption() const;

    /**
     * @brief Returns whether the port's caption should be visible
     * @return Is port caption visible
     */
    bool captionVisible() const;

    /**
     * @brief Returns whether the port is marked optional
     * @return Is port optional
     */
    bool optional() const;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_PORTEDITDIALOG_H
