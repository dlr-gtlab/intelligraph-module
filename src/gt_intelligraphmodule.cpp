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
#include "gt_intelligraphnodefactory.h"
#include "gt_intelligraphcategory.h"
#include "gt_intelligraphconnection.h"
#include "gt_intelligraphconnectiongroup.h"
#include "gt_intelligraphconnectionui.h"
#include "gt_intelligraphpackageui.h"
#include "gt_intelligraphnodeui.h"
#include "gt_intelligrapheditor.h"
#include "gt_igobjectlinkproperty.h"
#include "gt_igobjectlinkpropertyitem.h"
#include "gt_igstringselectionproperty.h"
#include "gt_igstringselectionpropertyitem.h"

static const int meta_port_index = [](){
    return qRegisterMetaType<gt::ig::PortIndex>("gt::ig::PortIndex");
}();
static const int meta_port_id = [](){
    return qRegisterMetaType<gt::ig::PortId>("gt::ig::PortId");
}();
static const int meta_node_id = [](){
    return qRegisterMetaType<gt::ig::NodeId>("gt::ig::NodeId");
}();

GtVersionNumber
GtIntelliGraphModule::version()
{
    return GtVersionNumber{0, 2, 0};
}

QString
GtIntelliGraphModule::description() const
{
    return QStringLiteral("GTlab IntelliGraph Module");
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

    m.author =    QStringLiteral("M. Bröcker, S. Reitenbach");
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
    static QVector<QByteArray> buffer;

    QMap<const char*, QMetaObject> map;

    map.insert(GT_CLASSNAME(GtIntelliGraphConnection),
               GT_METADATA(GtIntelliGraphConnectionUI));
    map.insert(GT_CLASSNAME(GtIntellIGraphConnectionGroup),
               GT_METADATA(GtIntelliGraphConnectionUI));

    map.insert(GT_CLASSNAME(GtIgPackage),
               GT_METADATA(GtIntelliGraphPackageUI));
    map.insert(GT_CLASSNAME(GtIntelliGraphCategory),
               GT_METADATA(GtIntelliGraphPackageUI));

    QStringList registeredNodes = GtIntelliGraphNodeFactory::instance().registeredNodes();
    buffer.reserve(registeredNodes.size());

    for (QString const& node : qAsConst(registeredNodes))
    {
        buffer.push_back(node.toLatin1());
        map.insert(buffer.constLast(), GT_METADATA(GtIntelliGraphNodeUI));
    }

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
    map.insert(GT_CLASSNAME(GtIgStringSelectionProperty),
               GT_METADATA(GtIgStringSelectionPropertyItem));

    return map;
}
