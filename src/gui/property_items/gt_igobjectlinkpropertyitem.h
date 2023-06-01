/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 31.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_IGOBJECTLINKPROPERTYITEM_H
#define GT_IGOBJECTLINKPROPERTYITEM_H

#include "gt_propertyitem.h"

class GtObjectLinkProperty;

/// stolen from "gt_propertyobjectlinkitem" for now

class GtIgObjectLinkPropertyItem : public GtPropertyItem
{
    Q_OBJECT

public:
    /**
     * @brief constructor
     */
    Q_INVOKABLE GtIgObjectLinkPropertyItem();

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

#endif // GT_IGOBJECTLINKPROPERTYITEM_H
