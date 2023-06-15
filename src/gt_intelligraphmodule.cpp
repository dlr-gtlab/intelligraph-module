/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 * 
 * Created on: 16.03.2023
 * Author: S. Reitenbach
 * Email: 
 */

/*
 * generated 1.2.0
 */
 
#include "gt_intelligraphmodule.h"

#include "gt_igpackage.h"
#include "gt_intelligraph.h"
#include "gt_intelligraphcategory.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphobjectui.h"
#include "gt_intelligrapheditor.h"
#include "gt_igobjectlinkproperty.h"
#include "gt_igobjectlinkpropertyitem.h"

#include "gt_igprojectui.h"

#include "gt_project.h"

GtVersionNumber
GtIntelliGraphModule::version()
{
    return GtVersionNumber{0, 1, 0};
}

QString
GtIntelliGraphModule::description() const
{
    return QStringLiteral("GTlab Nodes Module");
}

void
GtIntelliGraphModule::init()
{
    // nothing to do here
}

GtIntelliGraphModule::MetaInformation
GtIntelliGraphModule::metaInformation() const
{
    MetaInformation m;

    m.author =    QStringLiteral("S. Reitenbach, M. Bröcker");
    m.authorContact = QStringLiteral("AT-TWK");

    // TODO: set license
    // m.licenseShort = ...;

    return m;
}

QMetaObject
GtIntelliGraphModule::package()
{
    return GT_METADATA(GtIgPackage);
}

QList<QMetaObject>
GtIntelliGraphModule::data()
{
    QList<QMetaObject> list;

    list << GT_METADATA(GtIntelliGraph);
    list << GT_METADATA(GtIntelliGraphCategory);
    list << GT_METADATA(GtIntelliGraphConnection);

    return list;
}

bool
GtIntelliGraphModule::standAlone()
{
    return true;
}

QList<GtCalculatorData>
GtIntelliGraphModule::calculators()
{
    QList<GtCalculatorData> list;

    return list;
}

QList<GtTaskData>
GtIntelliGraphModule::tasks()
{
    QList<GtTaskData> list;

    return list;
}

QList<QMetaObject>
GtIntelliGraphModule::mdiItems()
{
    QList<QMetaObject> list;

    list << GT_METADATA(GtIntelliGraphEditor);

    return list;
}

QList<QMetaObject>
GtIntelliGraphModule::dockWidgets()
{
    QList<QMetaObject> list;

    return list;
}

QMap<const char*, QMetaObject>
GtIntelliGraphModule::uiItems()
{
    QMap<const char*, QMetaObject> map;

    map.insert(GT_CLASSNAME(GtProject),
               GT_METADATA(GtIgProjectUI));
    map.insert(GT_CLASSNAME(GtIgPackage),
               GT_METADATA(GtIntelliGraphObjectUI));
    map.insert(GT_CLASSNAME(GtIntelliGraph),
               GT_METADATA(GtIntelliGraphObjectUI));
    map.insert(GT_CLASSNAME(GtIntelliGraphCategory),
               GT_METADATA(GtIntelliGraphObjectUI));

    return map;
}

QList<QMetaObject>
GtIntelliGraphModule::postItems()
{
    QList<QMetaObject> list;

    return list;
}

QList<QMetaObject>
GtIntelliGraphModule::postPlots()
{
    QList<QMetaObject> list;

    return list;
}

QMap<const char*, QMetaObject>
GtIntelliGraphModule::propertyItems()
{
    QMap<const char*, QMetaObject> map;

    // not exported by default...
    map.insert(GT_CLASSNAME(GtIgObjectLinkProperty),
               GT_METADATA(GtIgObjectLinkPropertyItem));

    return map;
}
