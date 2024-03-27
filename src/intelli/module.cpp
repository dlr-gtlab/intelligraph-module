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
#include "intelli/gui/connectionui.h"
#include "intelli/gui/packageui.h"
#include "intelli/gui/nodeui.h"
#include "intelli/gui/grapheditor.h"
#include "intelli/gui/property_item/objectlink.h"
#include "intelli/gui/property_item/stringselection.h"
#include "intelli/gui/style.h"

#include "intelli/calculators/graphexeccalculator.h"

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

namespace intelli
{
gt::log::Stream& nospace(gt::log::Stream& s) { return s.nospace(); }
gt::log::Stream& space(gt::log::Stream& s) { return s.space(); }

gt::log::Stream& noquote(gt::log::Stream& s) { return s.noquote(); }
gt::log::Stream& quote(gt::log::Stream& s) { return s.quote(); }

gt::log::Stream& operator<<(gt::log::Stream& s, gt::log::Stream&(*f)(gt::log::Stream&)) { return f(s); }
}

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
static const int ns_meta_port_type = [](){
    return qRegisterMetaType<PortType>("intelli::PortType");
}();

GtVersionNumber
GtIntelliGraphModule::version()
{
    return GtVersionNumber{0, 9, 0};
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

    applyTheme(Theme::System);
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

    return routines;
}

QList<gt::SharedFunction>
GtIntelliGraphModule::sharedFunctions() const
{
    QList<gt::SharedFunction> list;

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

    QStringList registeredNodes = NodeFactory::instance().registeredNodes();
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

bool upgradeModuleFiles(QDomElement& root,
                        QString const& file,
                        gt::ConverterFunction f);

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
        object.setAttribute(QStringLiteral("class"), to);

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

// update dynamic input/output container types
bool upgrade_to_0_8_0_impl(QDomElement& root, QString const& file)
{
    auto objects = propertyContainerElements(root);

    if (objects.empty()) return true;

    for (auto& object : objects)
    {
        QString newType = QStringLiteral("PortDataIn");

        // check for dynamic node containers
        auto const& name = object.attribute(gt::xml::S_NAME_TAG);
        if (name == QStringLiteral("dynamicOutPorts"))
        {
            newType = QStringLiteral("PortDataOut");
        }
        else if (name != QStringLiteral("dynamicInPorts"))
        {
            continue;
        }

        QDomNodeList childs = object.childNodes();
        int size = childs.size();
        for (int i = 0; i < size; ++i)
        {
            auto child = childs.at(i).toElement();
            child.setAttribute(gt::xml::S_TYPE_TAG, newType);
        }
    }

    return true;
}

bool upgrade_to_0_8_0(QDomElement& root, QString const& file)
{
    return upgradeModuleFiles(root, file, upgrade_to_0_8_0_impl);
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

bool upgradeModuleFiles(QDomElement& root, QString const& file, gt::ConverterFunction f)
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
                    << "(XML ERROR: line:" << errorLine << intelli::nospace
                    << ", column: " << errorColumn << " -> " << errorStr;
                success = false;
                continue;
            }

            QDomElement root = document.documentElement();

            if (!f(root, filePath))
            {
                success = false;
                gtError()
                    << makeError()
                    << "(XML ERROR: line:" << errorLine << intelli::nospace
                    << ", column: " << errorColumn << " -> " << errorStr;
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
