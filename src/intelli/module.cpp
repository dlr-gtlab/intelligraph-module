/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

/*
 * generated 1.2.0
 */
 
#include "intelli/module.h"

#include "intelli/core.h"
#include "intelli/package.h"
#include "intelli/nodefactory.h"
#include "intelli/graph.h"
#include "intelli/graphcategory.h"
#include "intelli/graphuservariables.h"
#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/utilities.h"
#include "intelli/property/stringselection.h"
#include "intelli/node/binarydisplay.h"
#include "intelli/node/booldisplay.h"
#include "intelli/node/existingdirectorysource.h"
#include "intelli/node/finddirectchild.h"
#include "intelli/node/genericcalculatorexec.h"
#include "intelli/node/graphinputprovider.h"
#include "intelli/node/graphoutputprovider.h"
#include "intelli/node/logicoperation.h"
#include "intelli/node/numberdisplay.h"
#include "intelli/node/numbermath.h"
#include "intelli/node/objectsink.h"
#include "intelli/node/stringbuilder.h"
#include "intelli/node/stringselection.h"
#include "intelli/node/sleepy.h"
#include "intelli/node/textdisplay.h"
#include "intelli/node/genericcalculatorexec.h"
#include "intelli/node/input/boolinput.h"
#include "intelli/node/input/doubleinput.h"
#include "intelli/node/input/fileinput.h"
#include "intelli/node/input/intinput.h"
#include "intelli/node/input/objectinput.h"
#include "intelli/node/input/stringinput.h"
#include "intelli/gui/commentgroup.h"
#include "intelli/gui/commentdata.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/guidata.h"
#include "intelli/gui/graphui.h"
#include "intelli/gui/nodeui.h"
#include "intelli/gui/ui/commentui.h"
#include "intelli/gui/ui/connectionui.h"
#include "intelli/gui/ui/graphcategoryui.h"
#include "intelli/gui/ui/guidataui.h"
#include "intelli/gui/ui/packageui.h"
#include "intelli/gui/ui/node/boolnodeui.h"
#include "intelli/gui/ui/node/existingdirectorysourcenodeui.h"
#include "intelli/gui/ui/node/finddirectchildnodeui.h"
#include "intelli/gui/ui/node/genericcalculatorexecnodeui.h"
#include "intelli/gui/ui/node/logicnodeui.h"
#include "intelli/gui/ui/node/numberdisplaynodeui.h"
#include "intelli/gui/ui/node/numbermathnodeui.h"
#include "intelli/gui/ui/node/objectsinknodeui.h"
#include "intelli/gui/ui/node/stringbuildernodeui.h"
#include "intelli/gui/ui/node/stringselectionnodeui.h"
#include "intelli/gui/ui/node/sleepynodeui.h"
#include "intelli/gui/ui/node/textdisplaynodeui.h"
#include "intelli/gui/ui/node/fileinputnodeui.h"
#include "intelli/gui/ui/node/doubleinputnodeui.h"
#include "intelli/gui/ui/node/intinputnodeui.h"
#include "intelli/gui/ui/node/objectinputnodeui.h"
#include "intelli/gui/ui/node/stringinputnodeui.h"
#include "intelli/gui/property_item/stringselection.h"

#include "intelli/calculators/graphexeccalculator.h"

#include "intelli/private/upgrade_routines.h"

#include <gt_coreapplication.h>

using namespace intelli;
// non namespace variants
static const int meta_port_index = [](){
    return qRegisterMetaType<PortIndex>("PortIndex");
}();
static const int meta_port_id = [](){
    return qRegisterMetaType<PortId>("PortId");
}();
static const int meta_node_id = [](){
    return qRegisterMetaType<NodeId>("NodeId");
}();
static const int meta_node_uuid = [](){
    return qRegisterMetaType<NodeId>("NodeUuid");
}();
static const int meta_port_type = [](){
    return qRegisterMetaType<PortType>("PortType");
}();

// namespace variants
static const int ns_meta_port_index = [](){
    return qRegisterMetaType<PortIndex>("intelli::PortIndex");
}();
static const int ns_meta_port_id = [](){
    return qRegisterMetaType<PortId>("intelli::PortId");
}();
static const int ns_meta_node_id = [](){
    return qRegisterMetaType<NodeId>("intelli::NodeId");
}();
static const int ns_meta_node_uuid = [](){
    return qRegisterMetaType<NodeId>("intelli::NodeUuid");
}();
static const int ns_meta_port_type = [](){
    return qRegisterMetaType<PortType>("intelli::PortType");
}();

GtVersionNumber
GtIntelliGraphModule::version()
{
    return GtVersionNumber(0, 16, 0, "dev");
}

QString
GtIntelliGraphModule::description() const
{
    return QStringLiteral("GTlab IntelliGraph Module");
}

void
GtIntelliGraphModule::init()
{
    intelli::initModule();

    if (gtApp->batchMode()) return;
}

GtIntelliGraphModule::MetaInformation
GtIntelliGraphModule::metaInformation() const
{
    MetaInformation m;
    m.author = QString::fromUtf8(QByteArrayLiteral("M. Bröcker, S. Reitenbach"));
    m.authorContact = QStringLiteral("AT-TWK");
    m.licenseShort = QStringLiteral("BSD-3-Clause");

    return m;
}

QList<gt::VersionUpgradeRoutine>
GtIntelliGraphModule::upgradeRoutines() const
{
    QList<gt::VersionUpgradeRoutine> routines;

    gt::VersionUpgradeRoutine to_0_3_0;
    to_0_3_0.target = GtVersionNumber{0, 3, 0};
    to_0_3_0.f = upgrade_to_0_3_0;
    routines << to_0_3_0;

    gt::VersionUpgradeRoutine to_0_3_1;
    to_0_3_1.target = GtVersionNumber{0, 3, 1};
    to_0_3_1.f = upgrade_to_0_3_1;
    routines << to_0_3_1;

    gt::VersionUpgradeRoutine to_0_5_0;
    to_0_5_0.target = GtVersionNumber{0, 5, 0};
    to_0_5_0.f = upgrade_to_0_5_0;
    routines << to_0_5_0;

    gt::VersionUpgradeRoutine to_0_8_0;
    to_0_8_0.target = GtVersionNumber{0, 8, 0};
    to_0_8_0.f = upgrade_to_0_8_0;
    routines << to_0_8_0;

    gt::VersionUpgradeRoutine to_0_10_1;
    to_0_10_1.target = GtVersionNumber{0, 10, 1};
    to_0_10_1.f = upgrade_to_0_10_1;
    routines << to_0_10_1;

    gt::VersionUpgradeRoutine to_0_12_0;
    to_0_12_0.target = GtVersionNumber{0, 12, 0};
    to_0_12_0.f = upgrade_to_0_12_0;
    routines << to_0_12_0;

    gt::VersionUpgradeRoutine to_0_13_0;
    to_0_13_0.target = GtVersionNumber{0, 13, 0};
    to_0_13_0.f = upgrade_to_0_13_0;
    routines << to_0_13_0;

    gt::VersionUpgradeRoutine to_0_16_0_dev;
    to_0_16_0_dev.target = GtVersionNumber{0, 16, 0, "dev"};
    to_0_16_0_dev.f = upgrade_to_0_16_0_dev;
    routines << to_0_16_0_dev;

    return routines;
}

QList<gt::SharedFunction>
GtIntelliGraphModule::sharedFunctions() const
{
    auto calcWhiteList = gt::interface::makeSharedFunction(
        QStringLiteral("CalculatorNode_addToWhiteList"),
        GenericCalculatorExecNode::addToWhiteList,
        tr("Allows to register calculators that can be executed using\n"
           "the calculator execution node. Calculators must be registered\n"
           "explicitly. Signature: ") +
            gt::interface::getFunctionSignature(
                GenericCalculatorExecNode::addToWhiteList)
    );

    QList<gt::SharedFunction> list;
    list.append(calcWhiteList);
    return list;
}

QMetaObject
GtIntelliGraphModule::package()
{
    return GT_METADATA(Package);
}

QList<QMetaObject>
GtIntelliGraphModule::data()
{
    QList<QMetaObject> list;

    list << GT_METADATA(GraphCategory);
    list << GT_METADATA(ConnectionGroup);
    list << GT_METADATA(Connection);
    
    list << GT_METADATA(GraphUserVariables);

    list << GT_METADATA(GuiData);
    list << GT_METADATA(LocalStateContainer);

    list << GT_METADATA(CommentGroup);
    list << GT_METADATA(CommentData);

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

    auto graphExec = GT_CALC_DATA(intelli::GraphExecCalculator);
    graphExec->id = QStringLiteral("intelli graph execution");
    graphExec->version = GtVersionNumber(0, 1);
    graphExec->author = QStringLiteral("AT-TWK");
    graphExec->category = QStringLiteral("Graph");
    list << graphExec;

    return list;
}

QList<GtTaskData>
GtIntelliGraphModule::tasks()
{
    return {};
}

QList<QMetaObject>
GtIntelliGraphModule::mdiItems()
{
    QList<QMetaObject> list;
    
    list << GT_METADATA(GraphEditor);

    return list;
}

QList<QMetaObject>
GtIntelliGraphModule::dockWidgets()
{
    return {};
}

QMap<const char*, QMetaObject>
GtIntelliGraphModule::uiItems()
{
    // the nodes already need to be known
    intelli::initModule();

    static QVector<QByteArray> buffer;

    QMap<const char*, QMetaObject> map;

    map.insert(GT_CLASSNAME(Connection),
               GT_METADATA(ConnectionUI));
    map.insert(GT_CLASSNAME(ConnectionGroup),
               GT_METADATA(ConnectionUI));

    map.insert(GT_CLASSNAME(Package),
               GT_METADATA(PackageUI));
    map.insert(GT_CLASSNAME(GraphCategory),
               GT_METADATA(GraphCategoryUI));

    map.insert(GT_CLASSNAME(GuiData),
               GT_METADATA(GuiDataUI));
    map.insert(GT_CLASSNAME(LocalStateContainer),
               GT_METADATA(GuiDataUI));

    map.insert(GT_CLASSNAME(CommentGroup),
               GT_METADATA(CommentUI));
    map.insert(GT_CLASSNAME(CommentData),
               GT_METADATA(CommentUI));

    map.insert(GT_CLASSNAME(Graph),
               GT_METADATA(GraphUI));
    map.insert(GT_CLASSNAME(GraphInputProvider),
               GT_METADATA(GraphUI));
    map.insert(GT_CLASSNAME(GraphOutputProvider),
               GT_METADATA(GraphUI));

    map.insert(GT_CLASSNAME(LogicNode),
               GT_METADATA(LogicNodeUI));
    map.insert(GT_CLASSNAME(BinaryDisplayNode),
               GT_METADATA(LogicNodeUI));
    map.insert(GT_CLASSNAME(NumberDisplayNode),
               GT_METADATA(NumberDisplayNodeUI));
    map.insert(GT_CLASSNAME(NumberMathNode),
               GT_METADATA(NumberMathNodeUI));
    map.insert(GT_CLASSNAME(TextDisplayNode),
               GT_METADATA(TextDisplayNodeUI));

    map.insert(GT_CLASSNAME(BoolDisplayNode),
               GT_METADATA(BoolNodeUI));
    map.insert(GT_CLASSNAME(BoolInputNode),
               GT_METADATA(BoolNodeUI));
    map.insert(GT_CLASSNAME(FileInputNode),
               GT_METADATA(FileInputNodeUI));
    map.insert(GT_CLASSNAME(ObjectInputNode),
               GT_METADATA(ObjectInputNodeUI));
    map.insert(GT_CLASSNAME(StringInputNode),
               GT_METADATA(StringInputNodeUI));
    map.insert(GT_CLASSNAME(DoubleInputNode),
               GT_METADATA(DoubleInputNodeUI));
    map.insert(GT_CLASSNAME(IntInputNode),
               GT_METADATA(IntInputNodeUI));
    map.insert(GT_CLASSNAME(ExistingDirectorySourceNode),
               GT_METADATA(ExistingDirectorySourceNodeUI));
    map.insert(GT_CLASSNAME(FindDirectChildNode),
               GT_METADATA(FindDirectChildNodeUI));
    map.insert(GT_CLASSNAME(GenericCalculatorExecNode),
               GT_METADATA(GenericCalculatorExecNodeUI));
    map.insert(GT_CLASSNAME(ObjectSink),
               GT_METADATA(ObjectSinkNodeUI));
    map.insert(GT_CLASSNAME(StringBuilderNode),
               GT_METADATA(StringBuilderNodeUI));
    map.insert(GT_CLASSNAME(StringSelectionNode),
               GT_METADATA(StringSelectionNodeUI));
    map.insert(GT_CLASSNAME(SleepyNode),
               GT_METADATA(SleepyNodeUI));

    QStringList registeredNodes = NodeFactory::instance().registeredNodes();

    // remove all nodes with custom UI
    for (QString const& entry : utils::makeIterable(map.keyBegin(), map.keyEnd()))
    {
        registeredNodes.removeOne(entry);
    }

    buffer.reserve(registeredNodes.size());

    for (QString const& node : qAsConst(registeredNodes))
    {
        buffer.push_back(node.toLatin1());
        map.insert(buffer.constLast(), GT_METADATA(NodeUI));
    }

    return map;
}

QList<QMetaObject>
GtIntelliGraphModule::postItems()
{
    return {};
}

QList<QMetaObject>
GtIntelliGraphModule::postPlots()
{
    return {};
}

QMap<const char*, QMetaObject>
GtIntelliGraphModule::propertyItems()
{
    QMap<const char*, QMetaObject> map;

    map.insert(GT_CLASSNAME(StringSelectionProperty),
               GT_METADATA(StringSelectionPropertyItem));

    return map;
}
