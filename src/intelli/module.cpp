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
 
#include "intelli/module.h"

#include "intelli/core.h"
#include "intelli/package.h"
#include "intelli/nodefactory.h"
#include "intelli/graph.h"
#include "intelli/graphcategory.h"
#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/property/objectlink.h"
#include "intelli/property/stringselection.h"
#include "intelli/node/logicoperation.h"
#include "intelli/node/genericcalculatorexec.h"
#include "intelli/gui/ui/logicnodeui.h"
#include "intelli/gui/connectionui.h"
#include "intelli/gui/packageui.h"
#include "intelli/gui/nodeui.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/property_item/objectlink.h"
#include "intelli/gui/property_item/stringselection.h"

#include "intelli/calculators/graphexeccalculator.h"

#include <gt_logging.h>

#include "gt_xmlexpr.h"
#include "gt_xmlutilities.h"
#include "gt_coreapplication.h"

#include <QThread>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDomDocument>
#include <QDomNodeList>

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
    return GtVersionNumber(0, 12, 0);
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

    m.author =    QStringLiteral("M. Bröcker, S. Reitenbach");
    m.authorContact = QStringLiteral("AT-TWK");

    // TODO: set license
    // m.licenseShort = ...;

    return m;
}

bool upgrade_to_0_3_0(QDomElement& root, QString const& file);
bool upgrade_to_0_3_1(QDomElement& root, QString const& file);
bool upgrade_to_0_5_0(QDomElement& root, QString const& file);
bool upgrade_to_0_8_0(QDomElement& root, QString const& file);
bool upgrade_to_0_10_1(QDomElement& root, QString const& file);
bool upgrade_to_0_12_0(QDomElement& root, QString const& file);

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

    return routines;
}

QList<gt::SharedFunction>
GtIntelliGraphModule::sharedFunctions() const
{
    auto calcWhiteList = gt::interface::makeSharedFunction(
        QStringLiteral("CalculatorNode_addToWhiteList"),
        GenericCalculatorExecNode::addToWhiteList,
        tr("Allows to register calculators that can be executed using\n"
           "the calculator execution node.Calculators must be registered\n"
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
    QList<GtTaskData> list;

    return list;
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
    QList<QMetaObject> list;

    return list;
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
               GT_METADATA(PackageUI));

    map.insert(GT_CLASSNAME(LogicNode),
               GT_METADATA(LogicNodeUI));

    QStringList registeredNodes = NodeFactory::instance().registeredNodes();
    registeredNodes.removeOne(GT_CLASSNAME(LogicNode));

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
    map.insert(GT_CLASSNAME(ObjectLinkProperty),
               GT_METADATA(ObjectLinkPropertyItem));
    map.insert(GT_CLASSNAME(StringSelectionProperty),
               GT_METADATA(StringSelectionPropertyItem));

    return map;
}

template<typename ConverterFunction>
bool upgradeModuleFiles(QDomElement& root,
                        QString const& file,
                        ConverterFunction f);

// stolen from xml utilities
template <typename Predicate>
void
findElements(QDomElement const& elem,
             Predicate&& func,
             QList<QDomElement>& foundElements)
{
    if (func(elem))
    {
        foundElements.append(elem);
    }

    QDomElement child = elem.firstChildElement();
    while(!child.isNull())
    {
        findElements(child, func, foundElements);
        child = child.nextSiblingElement();
    }
}

// stolen from xml utilities
QList<QDomElement>
propertyContainerElements(QDomElement const& root)
{
    QList<QDomElement> result;
    findElements(root, [&](const QDomElement& elem) {
        return elem.tagName() == gt::xml::S_PROPERTYCONT_TAG;
    }, result);

    return result;
}

QDomElement
get_parent_object(QDomElement& object)
{
    return object
        .parentNode() // tag = objectlist
        .parentNode().toElement(); // tag = object
}

bool
rename_class_from_to(QDomElement& root,
                     QString const& from,
                     QString const& to,
                     int indent = 0,
                     std::function<void(QDomElement&, int)> func = {})
{
    auto objects = gt::xml::findObjectElementsByClassName(root, from);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Renaming %3 objects from '%1' to '%2'...").arg(from, to).arg(objects.size());

    for (auto& object : objects)
    {
        object.setAttribute(gt::xml::S_CLASS_TAG, to);

        if (func) func(object, indent + 2);
    }

    return true;
}

bool
replace_property_texts(QDomElement& root,
                       QString const& from,
                       QString const& to)
{
    auto objects = gt::xml::propertyElements(root);

    if (objects.empty()) return true;

    for (auto& object : objects)
    {
        auto text = object.firstChild().toText();
        if (text.isNull()) continue;

        if (text.data() == from)
        {
            text.setNodeValue(to);
        }
    }

    return true;
}

bool
replace_property_value(QDomElement& root,
                       QString const& propertyName,
                       QString const& newValue)
{
    auto objects = gt::xml::propertyElements(root);

    if (objects.empty()) return true;

    for (auto& object : objects)
    {
        if (object.attribute(gt::xml::S_NAME_TAG) == propertyName)
        {
            auto text = object.firstChild().toText();
            if (text.isNull()) continue;

            text.setNodeValue(newValue);
        }

    }

    return true;
}

QString
get_property_text(QDomElement& root,
                   QString const& propertyName)
{
    auto objects = gt::xml::propertyElements(root);
    if (objects.empty()) return {};

    for (auto& object : objects)
    {
        if (object.attribute(gt::xml::S_NAME_TAG) == propertyName)
        {
            auto text = object.firstChild().toText();
            return text.data();
        }
    }

    return {};
}

template<typename T> T
get_property_value(QDomElement& root,
                   QString const& propertyName)
{
    auto objects = gt::xml::propertyElements(root);
    if (objects.empty()) return T{};

    for (auto& object : objects)
    {
        if (object.attribute(gt::xml::S_NAME_TAG) == propertyName)
        {
            auto text = object.firstChild().toText();

            bool ok = true;
            T value{text.data().toUInt(&ok)};
            if (!ok) return T{};

            return value;
        }
    }

    return T{};
}

bool
remove_objects(QDomElement& root,
               QString const& className,
               int indent = 0)
{
    auto objects = gt::xml::findObjectElementsByClassName(root, className);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Removing %3 objects of type '%1'").arg(className).arg(objects.size());

    for (auto& object : objects)
    {
        object.parentNode().removeChild(object);
    }

    return true;
}

bool
replace_port_ids_in_connections(QDomElement graph,
                                NodeId nodeId,
                                PortId oldPortId,
                                PortId newPortId,
                                int indent = 0)
{
    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Updating connections for graph '%1'")
                    .arg(graph.toElement().attribute(gt::xml::S_NAME_TAG));

    indent++;

    // update connections in subgraph
    auto connectionGroup = graph
        .firstChildElement(gt::xml::S_OBJECTLIST_TAG)
        .firstChildElement(gt::xml::S_OBJECT_TAG);

    assert(connectionGroup.attribute(gt::xml::S_CLASS_TAG) == "intelli::ConnectionGroup");

    auto connections = gt::xml::findObjectElementsByClassName(connectionGroup, "intelli::Connection");
    for (auto connection : qAsConst(connections))
    {

        if (!connection.attribute(gt::xml::S_NAME_TAG).contains("updatedIn") &&
            get_property_value<NodeId>(connection, "inNodeId") == nodeId &&
            get_property_value<PortId>(connection, "inPort") == oldPortId)
        {
            gtInfo() << QStringLiteral(" ").repeated(indent)
                     << QObject::tr("Updating connection '%1' for node '%2'")
                            .arg(connection.attribute(gt::xml::S_NAME_TAG))
                            .arg(nodeId);

            replace_property_value(connection, "inPort", QString::number(newPortId));
            // hacky way to avoid updating the same connection twice. Name is regenerated once loaded
            connection.setAttribute(gt::xml::S_NAME_TAG, connection.attribute(gt::xml::S_NAME_TAG) + "updatedIn");
        }
        else if (!connection.attribute(gt::xml::S_NAME_TAG).contains("updatedOut") &&
                 get_property_value<NodeId>(connection, "outNodeId") == nodeId &&
                 get_property_value<PortId>(connection, "outPort") == oldPortId)
        {
            gtInfo() << QStringLiteral(" ").repeated(indent)
                     << QObject::tr("Updating connection '%1' for node '%2'")
                            .arg(connection.attribute(gt::xml::S_NAME_TAG))
                            .arg(nodeId);

            replace_property_value(connection, "outPort", QString::number(newPortId));
            // hacky way to avoid updating the same connection twice. Name is regenerated once loaded
            connection.setAttribute(gt::xml::S_NAME_TAG, connection.attribute(gt::xml::S_NAME_TAG) + "updatedOut");
        }
    }

    return true;
}


// update dynamic input/output container types
bool
rename_dynamic_ports_for_0_8_0(QDomElement& root,
                               QString const& file,
                               QString const& typeIn,
                               QString const& typeOut)
{
    auto containers = propertyContainerElements(root);

    for (auto& container : containers)
    {
        QString const* newType = &typeIn;

        // check for dynamic node containers
        auto const& name = container.attribute(gt::xml::S_NAME_TAG);
        if (name == QStringLiteral("dynamicOutPorts"))
        {
            newType = &typeOut;
        }
        else if (name != QStringLiteral("dynamicInPorts"))
        {
            continue;
        }

        QDomNodeList childs = container.childNodes();
        int size = childs.size();
        for (int i = 0; i < size; ++i)
        {
            auto child = childs.at(i).toElement();
            child.setAttribute(gt::xml::S_TYPE_TAG, *newType);
        }
    }

    return true;
}

bool
update_provider_ports_for_0_12_0(QDomElement& root,
                                 QString const& file,
                                 QString const& className,
                                 PortType portType)
{
    auto objects = gt::xml::findObjectElementsByClassName(root, className);
    if (objects.empty()) return true;

    gtInfo() << QObject::tr("Updating dynamic ports in '%1'")
                    .arg(file);

    int indent = 0;
    for (auto& provider : objects)
    {
        indent++;

        // access node id
        NodeId nodeId = get_property_value<NodeId>(provider, "id");
        assert(nodeId.isValid());

        gtInfo() << QStringLiteral(" ").repeated(indent)
                 << QObject::tr("Updating dynamic ports for '%1' (Node: %2)")
                        .arg(className).arg(nodeId);

        // iterate over all dynamic ports
        auto containers = propertyContainerElements(provider);
        for (auto& container : containers)
        {
            indent++;
            PortId newPortId = PortId::fromValue((size_t)portType + 1);

            // update ports
            QDomNodeList ports = container.childNodes();
            int nports = ports.size();
            for (int i = 0; i < nports; i++)
            {
                auto port = ports.at(i).toElement();

                // access  old port id
                PortId oldPortId = get_property_value<PortId>(port, "PortId");
                assert(oldPortId.isValid());

                gtInfo() << QStringLiteral(" ").repeated(indent)
                          << QObject::tr("Updating portId from '%1' to '%2'")
                                 .arg(oldPortId)
                                 .arg(newPortId);

                // update port id
                port.setAttribute("name", newPortId);
                replace_property_value(port, "PortId", QString::number(newPortId));

                // update connections in subgraph
                auto subgraph = get_parent_object(provider);
                assert(!subgraph.isNull());
                replace_port_ids_in_connections(subgraph, nodeId, oldPortId, newPortId, indent + 1);

                // update connections in parent graph
                auto rootgraph = get_parent_object(subgraph);
                assert(!rootgraph.isNull());

                NodeId subgraphId = get_property_value<NodeId>(subgraph, "id");
                // calculate port id of graph port
                PortId subgraphPortId = PortId::fromValue((size_t)(oldPortId << 1) | (size_t)invert(portType));

                replace_port_ids_in_connections(rootgraph, subgraphId, subgraphPortId, newPortId, indent + 1);

                // increment port id
                newPortId += PortId{4};
            }

            indent--;
        }

        indent--;
    }

    return true;
}

// remove dynamic ports since port id generation has changed
bool upgrade_to_0_12_0(QDomElement& root, QString const& file)
{
    return upgradeModuleFiles(
               root, file, std::bind(update_provider_ports_for_0_12_0,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     QStringLiteral("intelli::GroupInputProvider"),
                                     PortType::In)) &&
            upgradeModuleFiles(
               root, file, std::bind(update_provider_ports_for_0_12_0,
                                     std::placeholders::_1,
                                     std::placeholders::_2,
                                     QStringLiteral("intelli::GroupOutputProvider"),
                                     PortType::Out));
}

// rename dynamic port structs
bool upgrade_to_0_10_1(QDomElement& root, QString const& file)
{
    return upgradeModuleFiles(root, file, std::bind(rename_dynamic_ports_for_0_8_0,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    QStringLiteral("PortInfoIn"),
                                                    QStringLiteral("PortInfoOut")));
}

// rename dynamic port structs
bool upgrade_to_0_8_0(QDomElement& root, QString const& file)
{
    return upgradeModuleFiles(root, file, std::bind(rename_dynamic_ports_for_0_8_0,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    QStringLiteral("PortDataIn"),
                                                    QStringLiteral("PortDataOut")));
}

// connections no longer store indicies but port ids -> remove connections
bool upgrade_to_0_5_0(QDomElement& root, QString const& file)
{
    if (!file.contains(QStringLiteral("intelligraph"), Qt::CaseInsensitive)) return true;

    return remove_objects(root, GT_CLASSNAME(Connection));
}

// fix typo in class name :(
bool upgrade_to_0_3_1(QDomElement& root, QString const& file)
{
    if (!file.contains(QStringLiteral("intelligraph"), Qt::CaseInsensitive)) return true;

    return rename_class_from_to(root, QStringLiteral("intelli::NubmerDisplayNode"), QStringLiteral("intelli::NumberDisplayNode"));
}

// major refactoring of class names and namespaces
bool upgrade_to_0_3_0(QDomElement& root, QString const& file)
{
    if (!file.contains(QStringLiteral("intelligraph"), Qt::CaseInsensitive)) return true;

    int indent = 0;
    rename_class_from_to(root, QStringLiteral("GtIntelliGraphCategory"), GT_CLASSNAME(GraphCategory), indent, [](QDomElement& root, int indent){
        rename_class_from_to(root, QStringLiteral("GtIntelliGraph"), GT_CLASSNAME(Graph), indent, [](QDomElement& root, int indent){

            // connections
            rename_class_from_to(root, QStringLiteral("GtIntellIGraphConnectionGroup"), GT_CLASSNAME(ConnectionGroup), indent, [](QDomElement& root, int indent){
                rename_class_from_to(root, QStringLiteral("GtIntelliGraphConnection"), GT_CLASSNAME(Connection), indent);
            });

            // nodes
            rename_class_from_to(root, QStringLiteral("GtIgGroupInputProvider"), QStringLiteral("intelli::GroupInputProvider"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgGroupOutputProvider"), QStringLiteral("intelli::GroupOutputProvider"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgNubmerDisplayNode"), QStringLiteral("intelli::NubmerDisplayNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgNumberSourceNode"), QStringLiteral("intelli::NumberSourceNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgFindDirectChildNode"), QStringLiteral("intelli::FindDirectChildNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgObjectSourceNode"), QStringLiteral("intelli::ObjectSourceNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgObjectMementoNode"), QStringLiteral("intelli::ObjectMementoNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgStringListInputNode"), QStringLiteral("intelli::StringListInputNode"), indent);

            // dp
            rename_class_from_to(root, QStringLiteral("GtIgConditionalNode"), QStringLiteral("intelli::ConditionalNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgCheckDoubleNode"), QStringLiteral("intelli::CheckDoubleNode"), indent);
            rename_class_from_to(root, QStringLiteral("GtIgSleepyNode"), QStringLiteral("intelli::SleepyNode"), indent);

            // update dynamic in/out ports type ids
            replace_property_texts(root, QStringLiteral("GtIgDoubleData"), QStringLiteral("intelli::DoubleData"));
            replace_property_texts(root, QStringLiteral("GtIgStringListData"), QStringLiteral("intelli::StringListData"));
            replace_property_texts(root, QStringLiteral("GtIgObjectData"), QStringLiteral("intelli::ObjectData"));
            replace_property_texts(root, QStringLiteral("GtIgBoolData"), QStringLiteral("intelli::BoolData"));
        });
    });

    return true;
}

template<typename ConverterFunction>
bool upgradeModuleFiles(QDomElement& root, QString const& file, ConverterFunction f)
{
    if (!file.contains(QStringLiteral("intelligraph"), Qt::CaseSensitive)) return true;

    auto const makeError = [](){
        return QObject::tr("Failed to update intelligraph module data!");
    };

    QFileInfo info{file};
    QDir dir = info.absoluteDir();
    if (!dir.cd(Package::MODULE_DIR))
    {
        gtError() << makeError()
                  << QObject::tr("(Project directory '%1' does not exist)")
                         .arg(Package::MODULE_DIR);
        return false;
    }

    QDirIterator iter{
        dir.path(),
        QStringList{QStringLiteral("*")},
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDirIterator::NoIteratorFlags
    };

    bool success = true;

    while (iter.hasNext())
    {
        dir.cd(iter.next());

        QDirIterator fileIter{
            dir.path(),
            QStringList{'*' + Package::FILE_SUFFIX},
            QDir::Files,
            QDirIterator::NoIteratorFlags
        };

        while (fileIter.hasNext())
        {
            QString filePath = dir.absoluteFilePath(fileIter.next());
            QFile file{filePath};

            // stolen from Module Upgrader implementation
            QDomDocument document;
            QString errorStr;
            int errorLine;
            int errorColumn;

            if (!gt::xml::readDomDocumentFromFile(file, document, true,
                                                  &errorStr,
                                                  &errorLine,
                                                  &errorColumn))
            {
                gtError()
                    << makeError()
                    << "(XML ERROR: line:" << errorLine
                    << "- column:" << errorColumn << "->" << errorStr;
                success = false;
                continue;
            }

            QDomElement root = document.documentElement();

            if (!f(root, filePath))
            {
                success = false;
                gtError()
                    << makeError()
                    << "(XML ERROR: line:" << errorLine
                    << "- column:" << errorColumn << "->" << errorStr;
                continue;
            }

            // save file
            // new ordered attribute stream writer algorithm
            if (!gt::xml::writeDomDocumentToFile(filePath, document, true))
            {
                gtError()
                    << makeError()
                    << QObject::tr("(Failed to save graph flow '%1'!)")
                           .arg(file.fileName());
                success = false;
                continue;
            }
        }

        dir.cdUp();
    }

    return success;
}
