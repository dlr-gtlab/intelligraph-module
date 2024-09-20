/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTLINKPROPERTYITEM_H
#define GT_INTELLI_OBJECTLINKPROPERTYITEM_H

#include "gt_propertyitem.h"

class GtObjectLinkProperty;

namespace intelli
{

/// see "gt_propertyobjectlinkitem"

class ObjectLinkPropertyItem : public GtPropertyItem
{
    Q_OBJECT

public:
    /**
     * @brief constructor
     */
    Q_INVOKABLE ObjectLinkPropertyItem();

    /**
     * @brief data
     * @param column
     * @param role
     * @return
     */
    QVariant data(int column, int role) const override;

    /**
     * @brief setData
     * @param column
     * @param value
     * @param role
     * @return
     */
    bool setData(int column,
                 const QVariant& value,
                 GtObject* obj,
                 int role = Qt::EditRole) override;

    /**
     * @brief modeProperty
     * @return
     */
    GtObjectLinkProperty* objectLinkProperty() const;

    /**
     * @brief editorWidget
     * @return
     */
    QWidget* editorWidget(QWidget* parent,
                          const GtPropertyValueDelegate* delegate) const override;

    /**
     * @brief setEditorData
     * @param var
     */
    void setEditorData(QWidget* editor, QVariant& var) const override;

    /**
     * @brief setModelData
     * @param editor
     * @param model
     */
    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      const QModelIndex& index) const override;

    /**
     * @brief acceptDrop
     * @param mime data to check if it should be used
     * @return true if the mime data should be accepted
     */
    bool acceptDrop(const QMimeData* mime) const override;

    /**
     * @brief dropMimeData
     * handles mime data that are dropped
     * @param mime
     * @return true in case of success
     */
    bool dropMimeData(const QMimeData* mime) override;

};

} // namespace intelli

#endif // GT_INTELLI_OBJECTLINKPROPERTYITEM_H
