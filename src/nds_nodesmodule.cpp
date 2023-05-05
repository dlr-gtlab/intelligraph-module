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
#include "gt_intelligraph.h"
#include "gt_intelligraphcategory.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphobjectui.h"
#include "gt_intelligrapheditor.h"

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
    return QStringLiteral("GTlab Nodes Module");
}

void
NdsNodesModule::init()
{
    gtDebug() << "REGISTERED NODES:"
              << GtIntelliGraphNodeFactory::instance().knownClasses();
}

NdsNodesModule::MetaInformation
NdsNodesModule::metaInformation() const
{
    MetaInformation m;

    m.author =    QStringLiteral("S. Reitenbach, M. Bröcker");
    m.authorContact = QStringLiteral("AT-TWK");

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

    list << GT_METADATA(GtIntelliGraph);
    list << GT_METADATA(GtIntelliGraphCategory);
    list << GT_METADATA(GtIntelliGraphConnection);

    /* nodes */
    auto& factory = GtIntelliGraphNodeFactory::instance();
    auto const nodes = factory.knownClasses();
    list.reserve(list.size() + nodes.size());

    for (auto const& node : nodes)
    {
        if (auto* m = factory.metaObject(node)) list << *m;
    }

    return list;
}

bool
NdsNodesModule::standAlone()
{
    return true;
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

    list << GT_METADATA(GtIntelliGraphEditor);
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

    map.insert(GT_CLASSNAME(NdsPackage),
               GT_METADATA(GtIntelliGraphObjectUI));
    map.insert(GT_CLASSNAME(GtIntelliGraph),
               GT_METADATA(GtIntelliGraphObjectUI));
    map.insert(GT_CLASSNAME(GtIntelliGraphCategory),
               GT_METADATA(GtIntelliGraphObjectUI));

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

QMap<const char*, QMetaObject>
NdsNodesModule::propertyItems()
{
    QMap<const char*, QMetaObject> map;

    // not exported by default...
//    map.insert(GT_CLASSNAME(GtIgObjectLinkProperty),
//               GT_METADATA(GtPropertyObjectLinkItem));

    return map;
}
