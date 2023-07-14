/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GTIGSTRINGSELECTIONPROPERTYITEM_H
#define GTIGSTRINGSELECTIONPROPERTYITEM_H

#include "gt_propertyitem.h"
#include <QComboBox>

class GtIgComboBox : public QComboBox
{
    Q_OBJECT
public:

    using QComboBox::QComboBox;

protected:

    /**
     * @brief focusOutEvent overloaded to emit the focusOut events
     * @param event
     */
    void focusOutEvent(QFocusEvent* event) override;

signals:

    void focusOut();
};

class GtIgStringSelectionProperty;
class GtIgStringSelectionPropertyItem : public GtPropertyItem
{
    Q_OBJECT

public:

    Q_INVOKABLE GtIgStringSelectionPropertyItem();

    QVariant data(int column, int role) const override;

    bool setData(int column,
                 QVariant const& value,
                 GtObject* obj,
                 int role = Qt::EditRole) override;

    QWidget* editorWidget(QWidget* parent,
                          GtPropertyValueDelegate const* delegate) const override;

    void setEditorData(QWidget* editor, QVariant& var) const override;

    void setModelData(QWidget* editor,
                      QAbstractItemModel* model,
                      QModelIndex const& index) const override;

    GtIgStringSelectionProperty* property() const;
};

#endif // GTIGSTRINGSELECTIONPROPERTYITEM_H
