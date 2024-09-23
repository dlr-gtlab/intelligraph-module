/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/property_item/objectlink.h"

#include "intelli/property/objectlink.h"

#include "gt_propertyobjectlinkeditor.h"
#include "gt_datamodel.h"
#include "gt_objectfactory.h"

#include <QMimeData>

using namespace intelli;

ObjectLinkPropertyItem::ObjectLinkPropertyItem() = default;

QVariant
ObjectLinkPropertyItem::data(int column, int role) const
{
    if (!objectLinkProperty())
    {
        return QVariant();
    }

    if (column < 0 || column >= 3)
    {
        return QVariant();
    }

    if (column == 0)
    {
        return GtPropertyItem::data(column, role);
    }

    if (!m_property)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        if (column == 2)
        {
            const QString uuid = objectLinkProperty()->linkedObjectUUID();

            if (!scope())
            {
                return QVariant();
            }

            GtObject* linkedObject = scope()->getObjectByUuid(uuid);

            if (!linkedObject)
            {
                return QStringLiteral("-");
            }

            return linkedObject->objectName();
        }
    }
    }

    return GtPropertyItem::data(column, role);
}

bool
ObjectLinkPropertyItem::setData(int column, const QVariant& value,
                                  GtObject* obj, int role)
{
    if (column == 0)
    {
        GtPropertyItem::setData(column, value, obj, role);
        return true;
    }

    if (column != 2)
    {
        return false;
    }

    if (role != Qt::EditRole)
    {
        return false;
    }

    return GtPropertyItem::setData(column, value, obj, role);
}

GtObjectLinkProperty*
ObjectLinkPropertyItem::objectLinkProperty() const
{
    return qobject_cast<GtObjectLinkProperty*>(m_property);
}

QWidget*
ObjectLinkPropertyItem::editorWidget(
    QWidget* parent,
    const GtPropertyValueDelegate* /*delegate*/) const
{
    GtPropertyObjectLinkEditor* e = new GtPropertyObjectLinkEditor(parent);

    return e;
}

void
ObjectLinkPropertyItem::setEditorData(QWidget* editor,
                                        QVariant& /*var*/) const
{
    if (!objectLinkProperty())
    {
        return;
    }

    GtPropertyObjectLinkEditor* e =
        static_cast<GtPropertyObjectLinkEditor*>(editor);

    e->setScope(scope());
    e->setObjectLinkProperty(objectLinkProperty());
}

void
ObjectLinkPropertyItem::setModelData(QWidget* /*editor*/,
                                       QAbstractItemModel* /*model*/,
                                       const QModelIndex& /*index*/) const
{
    // nothing to do here
}

bool
ObjectLinkPropertyItem::acceptDrop(const QMimeData* mime) const
{
    if (!objectLinkProperty())
    {
        return false;
    }

    GtObject* obj = gtDataModel->objectFromMimeData(mime, false,
                                                    gtObjectFactory);

    return obj != nullptr && objectLinkProperty()->allowedClasses().contains(
               obj->metaObject()->className());
}

bool
ObjectLinkPropertyItem::dropMimeData(const QMimeData* mime)
{
    if (!objectLinkProperty())
    {
        return false;
    }

    GtObject* obj = gtDataModel->objectFromMimeData(mime, false,
                                                    gtObjectFactory);

    if (obj && objectLinkProperty()->allowedClasses().contains(
            obj->metaObject()->className()))
    {
        objectLinkProperty()->setVal(obj->uuid());
        return true;
    }

    return false;
}
