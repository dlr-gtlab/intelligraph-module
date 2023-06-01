/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 31.5.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#include "gt_igobjectlinkpropertyitem.h"

#include "gt_igobjectlinkproperty.h"
#include "gt_propertyobjectlinkeditor.h"
#include "gt_datamodel.h"
#include "gt_objectfactory.h"

#include <QMimeData>

GtIgObjectLinkPropertyItem::GtIgObjectLinkPropertyItem() = default;

QVariant
GtIgObjectLinkPropertyItem::data(int column, int role) const
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
GtIgObjectLinkPropertyItem::setData(int column, const QVariant& value,
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
GtIgObjectLinkPropertyItem::objectLinkProperty() const
{
    return qobject_cast<GtObjectLinkProperty*>(m_property);
}

QWidget*
GtIgObjectLinkPropertyItem::editorWidget(
    QWidget* parent,
    const GtPropertyValueDelegate* /*delegate*/) const
{
    GtPropertyObjectLinkEditor* e = new GtPropertyObjectLinkEditor(parent);

    return e;
}

void
GtIgObjectLinkPropertyItem::setEditorData(QWidget* editor,
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
GtIgObjectLinkPropertyItem::setModelData(QWidget* /*editor*/,
                                       QAbstractItemModel* /*model*/,
                                       const QModelIndex& /*index*/) const
{
    // nothing to do here
}

bool
GtIgObjectLinkPropertyItem::acceptDrop(const QMimeData* mime) const
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
GtIgObjectLinkPropertyItem::dropMimeData(const QMimeData* mime)
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
