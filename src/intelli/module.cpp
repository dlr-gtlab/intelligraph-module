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
#include "intelli/connection.h"
#include "intelli/connectiongroup.h"
#include "intelli/property/stringselection.h"
#include "intelli/node/logicoperation.h"
#include "intelli/node/genericcalculatorexec.h"
#include "intelli/gui/ui/logicnodeui.h"
#include "intelli/gui/ui/connectionui.h"
#include "intelli/gui/ui/packageui.h"
#include "intelli/gui/ui/graphcategoryui.h"
#include "intelli/gui/nodeui.h"
#include "intelli/gui/grapheditor.h"
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
    return GtVersionNumber(0, 13, 0, "dev1");
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
    m.licenseShort = QStringLiteral("BSD-3-Clause");

    return m;
}

bool upgrade_to_0_3_0(QDomElement& root, QString const& file);
bool upgrade_to_0_3_1(QDomElement& root, QString const& file);
bool upgrade_to_0_5_0(QDomElement& root, QString const& file);
bool upgrade_to_0_8_0(QDomElement& root, QString const& file);
bool upgrade_to_0_10_1(QDomElement& root, QString const& file);
bool upgrade_to_0_12_0(QDomElement& root, QString const& file);
bool upgrade_to_0_13_0(QDomElement& root, QString const& file);

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
    to_0_13_0.target = GtVersionNumber{0, 13, 0, "dev1"};
    to_0_13_0.f = upgrade_to_0_13_0;
    routines << to_0_13_0;

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

using ConverterFunction = std::function<bool(QDomElement&, QString const&)>;
using ConverterFunctions = std::initializer_list<ConverterFunction>;

using ConversionStrategy = std::function<void(QDomElement&, int)>;

bool upgradeModuleFiles(QDomElement&, QString const&, std::initializer_list<ConverterFunction>);
bool upgradeModuleFiles(QDomElement& d, QString const& s, ConverterFunction f);

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

QString
get_property_text(QDomElement& property)
{
    auto text = property.firstChild().toText();
    return text.data();
}

template<typename T> T
get_property_value(QDomElement& root,
                   QString const& propertyName)
{
    bool ok = true;
    auto value = get_property_text(root, propertyName).toUInt(&ok);
    return ok ? T{value} : T{};
}

bool
rename_class_from_to(QDomElement& root,
                     QString const& file,
                     QString const& from,
                     QString const& to,
                     int indent = 0,
                     ConversionStrategy func = {})
{
    auto objects = gt::xml::findObjectElementsByClassName(root, from);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Renaming %4 objects from '%1' to '%2'... (file: %3")
                    .arg(from, to, file)
                    .arg(objects.size());

    for (auto& object : objects)
    {
        object.setAttribute(gt::xml::S_CLASS_TAG, to);

        if (func) func(object, indent + 2);
    }

    return true;
}

bool
rename_class_from_to_v0(QDomElement& root,
                        QString const& from,
                        QString const& to,
                        int indent = 0,
                        ConversionStrategy func = {})
{
    return rename_class_from_to(root, {}, from, to, indent, std::move(func));
}

// updates the ident of all properties from `oldIdent` to `newIdent`
bool
replace_property_idents_of_class(QDomElement& root,
                                 QString const& file,
                                 QString const& className,
                                 QString const& oldIdent,
                                 QString const& newIdent,
                                 int indent = 0)
{
    auto objects = gt::xml::findObjectElementsByClassName(root, className);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Updating properties indents for class '%1'... (file: %2)")
                    .arg(className, file);

    indent++;

    for (auto& object : objects)
    {
        auto properties = gt::xml::propertyElements(object);

        for (auto& property : properties)
        {
            if (property.attribute(gt::xml::S_NAME_TAG) == oldIdent)
            {
                property.setAttribute(gt::xml::S_NAME_TAG, newIdent);
                continue; // property ident should only exists once
            }
        }
    }

    return true;
}

// replaces the `property`'s value with `value`
void
replace_property_value(QDomElement& property,
                       QString const& value)
{
    auto text = property.firstChild().toText();
    text.setNodeValue(value);
}

// replaces all properties with `to` that contain `from` as a value
bool
replace_property_values(QDomElement& root,
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

// replaces the value of all properties named `propertyName` with `newValue`
bool
replace_value_of_property(QDomElement& root,
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

/// appends a property to `root` with the id `propertyId` and the default value
/// of `defaultValue`
void
add_property(QDomElement& root,
             QString const& propertyId,
             QString const& defaultValue,
             int indent = 0)
{
    Q_UNUSED(indent);

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Adding property '%1'...")
                    .arg(propertyId);

    auto doc = root.ownerDocument();

    auto property = gt::xml::createStringPropertyElement(doc, propertyId, defaultValue);
    root.appendChild(property);
}

bool
replace_mode_property_of_class(QDomElement& root,
                               QString const& file,
                               QString const& className,
                               QString const& propertyId,
                               QMap<QString, QString> const& map,
                               QString const& defaultValue,
                               int indent = 0)
{
    auto objects = gt::xml::findObjectElementsByClassName(root, className);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Updating mode properties for class '%1'... (file: %2)")
                    .arg(className, file);

    indent += 2;

    for (auto& object : objects)
    {
        auto properties = gt::xml::propertyElements(object);

        for (auto& property : properties)
        {
            if (property.attribute(gt::xml::S_NAME_TAG) == propertyId)
            {
                QString oldValue = get_property_text(property);
                QString newValue = map.value(oldValue, defaultValue);

                gtInfo() << QStringLiteral(" ").repeated(indent)
                         << QObject::tr("Replacing '%1' with '%2'")
                                .arg(oldValue, newValue);

                replace_property_value(property, newValue);
                continue; // property ident should only exists once
            }
        }
    }

    return true;
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
replace_port_ids_in_connections(QDomElement& graph,
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

            replace_value_of_property(connection, "inPort", QString::number(newPortId));
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

            replace_value_of_property(connection, "outPort", QString::number(newPortId));
            // hacky way to avoid updating the same connection twice. Name is regenerated once loaded
            connection.setAttribute(gt::xml::S_NAME_TAG, connection.attribute(gt::xml::S_NAME_TAG) + "updatedOut");
        }
    }

    return true;
}

bool
replace_port_ids_in_connections_by_class(QDomElement& root,
                                         QString const& file,
                                         QString const& className,
                                         PortId oldPortId,
                                         PortId newPortId,
                                         int indent = 0)
{
    auto objects = gt::xml::findObjectElementsByClassName(root, className);

    if (objects.empty()) return true;

    gtInfo() << QStringLiteral(" ").repeated(indent)
             << QObject::tr("Updating connections for class '%1'... (file: %2)")
                    .arg(className, file);

    indent++;

    for (auto& object : objects)
    {
        auto parent = get_parent_object(object);
        if (parent.isNull()) continue;

        // access node id
        NodeId nodeId = get_property_value<NodeId>(object, "id");
        assert(nodeId.isValid());

        replace_port_ids_in_connections(parent, nodeId, oldPortId, newPortId, indent + 1);
    }

    return true;
}


// update dynamic input/output container types
bool
// cppcheck-suppress constParameterCallback
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
                replace_value_of_property(port, "PortId", QString::number(newPortId));

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

// removed redundant input nodes
bool upgrade_to_0_13_0(QDomElement& root, QString const& file)
{
    constexpr int indent = 0;

    // mapping of mode types for double/int input nodes
    QMap<QString, QString> map;
    map.insert(QStringLiteral("Text"),    QStringLiteral("LineEditBound"));
    map.insert(QStringLiteral("dial"),    QStringLiteral("Dial"));
    map.insert(QStringLiteral("sliderH"), QStringLiteral("SliderH"));
    map.insert(QStringLiteral("sliderV"), QStringLiteral("SliderV"));

    return upgradeModuleFiles(
        root,
        file,
        {
            // ObjectSourceNode replaced with ObjectInputNode, output id changed
            std::bind(replace_port_ids_in_connections_by_class,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::ObjectSourceNode"),
                      PortId(1),
                      PortId(0),
                      indent),
            // ObjectSourceNode replaced with ObjectInputNode
            std::bind(rename_class_from_to,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::ObjectSourceNode"),
                      QStringLiteral("intelli::ObjectInputNode"),
                      indent,
                      nullptr),
            // property name of ObjectInputNode replaced
            std::bind(replace_property_idents_of_class,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::ObjectInputNode"),
                      QStringLiteral("value"),
                      QStringLiteral("target"),
                      indent),
            // logic source replaced by bool input node
            std::bind(rename_class_from_to,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::LogicSourceNode"),
                      QStringLiteral("intelli::BoolInputNode"),
                      indent,
                      // make bool input node use button as a widget
                      ConversionStrategy(std::bind(add_property,
                                                   std::placeholders::_1,
                                                   QStringLiteral("displayMode"),
                                                   QStringLiteral("Button"),
                                                   std::placeholders::_2))),
            // logic display replaced by bool display node
            std::bind(rename_class_from_to,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::LogicDisplayNode"),
                      QStringLiteral("intelli::BoolDisplayNode"),
                      indent,
                      // make bool display node use button as a widget
                      ConversionStrategy(std::bind(add_property,
                                                   std::placeholders::_1,
                                                   QStringLiteral("displayMode"),
                                                   QStringLiteral("Button"),
                                                   std::placeholders::_2))),
            // logic display replaced by bool display node
            std::bind(rename_class_from_to,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::NumberSourceNode"),
                      QStringLiteral("intelli::DoubleInputNode"),
                      indent,
                      nullptr),
         // mode values of of input type changed
         std::bind(replace_mode_property_of_class,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   QStringLiteral("intelli::DoubleInputNode"),
                   QStringLiteral("type"),
                   map,
                   QStringLiteral("LineEditBound"),
                   indent),
         std::bind(replace_mode_property_of_class,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   QStringLiteral("intelli::IntInputNode"),
                   QStringLiteral("type"),
                   map,
                   QStringLiteral("LineEditBound"),
                   indent),
         // property name of input type changed
         std::bind(replace_property_idents_of_class,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   QStringLiteral("intelli::DoubleInputNode"),
                   QStringLiteral("type"),
                   QStringLiteral("mode"),
                   indent),
         std::bind(replace_property_idents_of_class,
                   std::placeholders::_1,
                   std::placeholders::_2,
                   QStringLiteral("intelli::IntInputNode"),
                   QStringLiteral("type"),
                   QStringLiteral("mode"),
                   indent)
        });
}

// remove dynamic ports since port id generation has changed
bool upgrade_to_0_12_0(QDomElement& root, QString const& file)
{
    return upgradeModuleFiles(
        root,
        file,
        {
            std::bind(update_provider_ports_for_0_12_0,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::GroupInputProvider"),
                      PortType::In),
            std::bind(update_provider_ports_for_0_12_0,
                      std::placeholders::_1,
                      std::placeholders::_2,
                      QStringLiteral("intelli::GroupOutputProvider"),
                      PortType::Out)
        });
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

    return rename_class_from_to_v0(root, QStringLiteral("intelli::NubmerDisplayNode"), QStringLiteral("intelli::NumberDisplayNode"));
}

// major refactoring of class names and namespaces
bool upgrade_to_0_3_0(QDomElement& root, QString const& file)
{
    if (!file.contains(QStringLiteral("intelligraph"), Qt::CaseInsensitive)) return true;

    int indent = 0;
    rename_class_from_to_v0(root, QStringLiteral("GtIntelliGraphCategory"), GT_CLASSNAME(GraphCategory), indent, [](QDomElement& root, int indent){
        rename_class_from_to_v0(root, QStringLiteral("GtIntelliGraph"), GT_CLASSNAME(Graph), indent, [](QDomElement& root, int indent){

            // connections
            rename_class_from_to_v0(root, QStringLiteral("GtIntellIGraphConnectionGroup"), GT_CLASSNAME(ConnectionGroup), indent, [](QDomElement& root, int indent){
                rename_class_from_to_v0(root, QStringLiteral("GtIntelliGraphConnection"), GT_CLASSNAME(Connection), indent);
            });

            // nodes
            rename_class_from_to_v0(root, QStringLiteral("GtIgGroupInputProvider"), QStringLiteral("intelli::GroupInputProvider"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgGroupOutputProvider"), QStringLiteral("intelli::GroupOutputProvider"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgNubmerDisplayNode"), QStringLiteral("intelli::NubmerDisplayNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgNumberSourceNode"), QStringLiteral("intelli::NumberSourceNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgFindDirectChildNode"), QStringLiteral("intelli::FindDirectChildNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgObjectSourceNode"), QStringLiteral("intelli::ObjectSourceNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgObjectMementoNode"), QStringLiteral("intelli::ObjectMementoNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgStringListInputNode"), QStringLiteral("intelli::StringListInputNode"), indent);

            // dp
            rename_class_from_to_v0(root, QStringLiteral("GtIgConditionalNode"), QStringLiteral("intelli::ConditionalNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgCheckDoubleNode"), QStringLiteral("intelli::CheckDoubleNode"), indent);
            rename_class_from_to_v0(root, QStringLiteral("GtIgSleepyNode"), QStringLiteral("intelli::SleepyNode"), indent);

            // update dynamic in/out ports type ids
            replace_property_values(root, QStringLiteral("GtIgDoubleData"), QStringLiteral("intelli::DoubleData"));
            replace_property_values(root, QStringLiteral("GtIgStringListData"), QStringLiteral("intelli::StringListData"));
            replace_property_values(root, QStringLiteral("GtIgObjectData"), QStringLiteral("intelli::ObjectData"));
            replace_property_values(root, QStringLiteral("GtIgBoolData"), QStringLiteral("intelli::BoolData"));
        });
    });

    return true;
}

bool upgradeModuleFiles(QDomElement& d, QString const& s, ConverterFunction f)
{
    return upgradeModuleFiles(d, s, {f});
}

bool upgradeModuleFiles(QDomElement& /*root*/,
                        QString const& moduleFilePath,
                        std::initializer_list<ConverterFunction> funcs)
{
    if (!moduleFilePath.contains(QStringLiteral("intelligraph"), Qt::CaseSensitive)) return true;

    auto const makeError = [](){
        return QObject::tr("Failed to update intelligraph module data!");
    };

    QFileInfo info{moduleFilePath};
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
        if (!dir.cd(iter.next()))
        {
            gtWarning() << makeError()
                        << QObject::tr("(Category directory '%1' does not exist)")
                               .arg(iter.path());
            continue;
        }

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

            // see Module Upgrader implementation
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

            bool doContinue = false;

            for (auto f : funcs)
            {
                if (!f(root, filePath))
                {
                    success = false;
                    gtError()
                        << makeError()
                        << "(XML ERROR: line:" << errorLine
                        << "- column:" << errorColumn << "->" << errorStr;
                    doContinue = true;
                    break;
                }
            }

            if (doContinue) continue;

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
