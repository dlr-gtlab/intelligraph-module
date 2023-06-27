/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igstringselectionpropertyitem.h"
#include "gt_igstringselectionproperty.h"
#include "gt_propertyvaluedelegate.h"

#include <QVariant>
#include <QComboBox>

void
GtIgComboBox::focusOutEvent(QFocusEvent* event)
{
    emit focusOut();
    QComboBox::focusOutEvent(event);
}

GtIgStringSelectionPropertyItem::GtIgStringSelectionPropertyItem() = default;

GtIgStringSelectionProperty*
GtIgStringSelectionPropertyItem::property() const
{
    return qobject_cast<GtIgStringSelectionProperty*>(m_property);
}

QVariant
GtIgStringSelectionPropertyItem::data(int column, int role) const
{
    if (!property())
    {
        return {};
    }

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        if (column == 2)
        {
            return property()->selectedValue();
        }
    }
    }

    return GtPropertyItem::data(column, role);
}

bool
GtIgStringSelectionPropertyItem::setData(int column,
                                         QVariant const& value,
                                         GtObject* obj,
                                         int role)
{
    return GtPropertyItem::setData(column, value, obj, role);
}

QWidget*
GtIgStringSelectionPropertyItem::editorWidget(QWidget* parent,
                                              GtPropertyValueDelegate const* delegate) const
{
    auto* selection = new GtIgComboBox(parent);
    if (auto* p = property())
    {
        selection->addItems(property()->values());
        selection->setCurrentText(property()->selectedValue());
        connect(selection, &GtIgComboBox::focusOut, p, [=](){
            p->select(selection->currentIndex());
        });
    }
    return selection;
}

void
GtIgStringSelectionPropertyItem::setEditorData(QWidget* editor, QVariant& var) const
{
    return;
}

void
GtIgStringSelectionPropertyItem::setModelData(QWidget*,
                                              QAbstractItemModel*,
                                              QModelIndex const&) const
{
    // nothing to do here
}
