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
 
#include "nds_nodesmodule.h"

#include "nds_package.h"
#include "nds_nodeeditor.h"
#include "nds_projectui.h"
#include "nds_3dplot.h"

#include "gt_project.h"

GtVersionNumber
NdsNodesModule::version()
{
    return GtVersionNumber{0, 0, 1};
}

QString
NdsNodesModule::description() const
{
    return QStringLiteral("Nds Nodes Module");
}

void
NdsNodesModule::init()
{
    // TODO: code to execute on init
}

NdsNodesModule::MetaInformation
NdsNodesModule::metaInformation() const
{
    MetaInformation m;

    m.author =    QStringLiteral("S. Reitenbach");
    m.authorContact = QStringLiteral("");

    // TODO: set license
    // m.licenseShort = ...;

    return m;
}

QMetaObject
NdsNodesModule::package()
{
    return GT_METADATA(NdsPackage);
}

QList<QMetaObject>
NdsNodesModule::data()
{
    QList<QMetaObject> list;

    return list;
}

bool
NdsNodesModule::standAlone()
{
    return false;
}

QList<GtCalculatorData>
NdsNodesModule::calculators()
{
    QList<GtCalculatorData> list;

    return list;
}

QList<GtTaskData>
NdsNodesModule::tasks()
{
    QList<GtTaskData> list;

    return list;
}

QList<QMetaObject>
NdsNodesModule::mdiItems()
{
    QList<QMetaObject> list;

    list << GT_METADATA(NdsNodeEditor);
    list << GT_METADATA(Nds3DPlot);

    return list;
}

QList<QMetaObject>
NdsNodesModule::dockWidgets()
{
    QList<QMetaObject> list;

    return list;
}

QMap<const char*, QMetaObject>
NdsNodesModule::uiItems()
{
    QMap<const char*, QMetaObject> map;

    map.insert(GT_CLASSNAME(GtProject),
               GT_METADATA(NdsProjectUI));

    return map;
}

QList<QMetaObject>
NdsNodesModule::postItems()
{
    QList<QMetaObject> list;

    return list;
}

QList<QMetaObject>
NdsNodesModule::postPlots()
{
    QList<QMetaObject> list;

    return list;
}
