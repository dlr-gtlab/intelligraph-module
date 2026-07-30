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

    explicit PortEditDialog(PortType portType, QStringList const& typeWhiteList = {});
    ~PortEditDialog();

    void setTypeId(TypeId const& typeId);
    void setCaption(QString const& caption);
    void setCaptionVisible(bool visible = true);

    TypeId typeId() const;
    QString caption() const;
    bool captionVisible() const;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_PORTEDITDIALOG_H
