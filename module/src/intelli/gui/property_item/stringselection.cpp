/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 27.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "intelli/gui/property_item/stringselection.h"
#include "intelli/property/stringselection.h"

#include "gt_propertyvaluedelegate.h"

#include <QVariant>
#include <QComboBox>

using namespace intelli;

void
ComboBox::focusOutEvent(QFocusEvent* event)
{
    emit focusOut();
    QComboBox::focusOutEvent(event);
}

StringSelectionPropertyItem::StringSelectionPropertyItem() = default;

StringSelectionProperty*
StringSelectionPropertyItem::property() const
{
    return qobject_cast<StringSelectionProperty*>(m_property);
}

QVariant
StringSelectionPropertyItem::data(int column, int role) const
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
StringSelectionPropertyItem::setData(int column,
                                         QVariant const& value,
                                         GtObject* obj,
                                         int role)
{
    return GtPropertyItem::setData(column, value, obj, role);
}

QWidget*
StringSelectionPropertyItem::editorWidget(QWidget* parent,
                                              GtPropertyValueDelegate const* delegate) const
{
    auto* selection = new ComboBox(parent);
    if (auto* p = property())
    {
        selection->addItems(property()->values());
        selection->setCurrentText(property()->selectedValue());
        connect(selection, &ComboBox::focusOut, p, [=](){
            p->select(selection->currentIndex());
        });
    }
    return selection;
}

void
StringSelectionPropertyItem::setEditorData(QWidget* editor, QVariant& var) const
{
    return;
}

void
StringSelectionPropertyItem::setModelData(QWidget*,
                                          QAbstractItemModel*,
                                          QModelIndex const&) const
{
    // nothing to do here
}
