/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */

#include <intelli/node/genericcalculatorexec.h>

#include <intelli/data/object.h>
#include <intelli/data/double.h>
#include <intelli/data/bool.h>
#include <intelli/data/string.h>
#include <intelli/data/integer.h>

#include "gt_algorithms.h"
#include "gt_coreapplication.h"
#include "gt_project.h"
#include "gt_calculator.h"
#include "gt_calculatorfactory.h"

#include "gt_objectpathproperty.h"
#include "gt_processfactory.h"
#include "gt_doubleproperty.h"
#include "gt_boolproperty.h"
#include "gt_intproperty.h"
#include "gt_objectlinkproperty.h"
#include "gt_doublemonitoringproperty.h"
#include "gt_intmonitoringproperty.h"
#include "gt_stringmonitoringproperty.h"

#include "gt_stylesheets.h"
#include "gt_propertytreeview.h"

#include <QVBoxLayout>
#include <QComboBox>
#include <QCompleter>

using namespace intelli;

struct PortTypeHelper
{
    TypeId typeId = {};
    /// whether to add the port as input or output
    bool addInPort = false, addOutPort = false;

    operator bool() const { return !typeId.isEmpty(); }
};

/// Maps property class names onto port types
/// and denotes whether an input or output should be generated
static QHash<char const*, PortTypeHelper> s_portFromPropertyMap;

using VariantFromPortDataFunctor =
    std::function<QVariant(NodeDataPtr, GtAbstractProperty&, GtCalculator&)>;
/// Maps port type ids to functor for retrieving the variant data
static QHash<TypeId, VariantFromPortDataFunctor> s_variantFromPortDataMap;

using VariantToPortDataFunctor =
    std::function<NodeDataPtr(QVariant, GtAbstractProperty&, GtCalculator&)>;
/// Maps port type ids to functor for retrieving the node data of a property
static QHash<TypeId, VariantToPortDataFunctor> s_variantToPortDataMap;

using ClassName = QString;
using ClassIdent = QString;

/// Maps class names of calculators to calculator idents
QMap<ClassName, ClassIdent> s_knownClasses;

struct GenericCalculatorExecNode::Impl
{
    template<typename Ports>
    static inline bool
    setPortPropertyHidden(GenericCalculatorExecNode& node,
                          Ports& ports,
                          PortId portId,
                          bool hide)
    {
        if (!ports.contains(portId)) return false;

        if (auto obj = node.currentObject())
        {
            if (auto* prop = obj->findProperty(ports[portId]))
            {
                prop->hide(hide);
                emit node.currentObjectChanged();
                return true;
            }
        }

        return false;
    }

    /// generates a standardized port caption from a property
    static inline QString
    generatePortCaption(GtAbstractProperty const& prop)
    {
        return prop.objectName().toLower().replace(" ", "_");
    }

    static inline PortTypeHelper
    propertyToPortType(char const* className)
    {
        return s_portFromPropertyMap.value(className);
    }

    static inline VariantFromPortDataFunctor
    variantFromPortData(QString const& className)
    {
        return s_variantFromPortDataMap.value(className);
    }

    static inline VariantToPortDataFunctor
    variantToPortData(QString const& className)
    {
        return s_variantToPortDataMap.value(className);
    }

    static inline QStringList const&
    classNames()
    {
        static QStringList keys;
        // known classes may not be initialized yet
        if (keys.empty()) keys = s_knownClasses.keys();
        return keys;
    }

    static inline QStringList const&
    classIndents()
    {
        static QStringList values;
        // known classes may not be initialized yet
        if (values.empty()) values = s_knownClasses.values();
        return values;
    }

    static inline ClassIdent
    classNameToIdent(ClassName const& className)
    {
        return s_knownClasses.value(className);
    }

    static inline ClassName
    identToClassName(ClassIdent const& identName)
    {
        return s_knownClasses.key(identName);
    }

    static inline bool
    isKnownClass(ClassName const& className)
    {
        return classNames().contains(className);
    }

    /// mapping functions that return the appropiate port type id
    static inline void initPortFromPropertyMap()
    {
        // monitoring properties are outgoing
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtDoubleMonitoringProperty),
                                     { typeId<DoubleData>() }
                                     )->addOutPort = true;
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtIntMonitoringProperty),
                                     { typeId<IntData>() }
                                     )->addOutPort = true;
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtStringMonitoringProperty),
                                     { typeId<StringData>() }
                                     )->addOutPort = true;
        // inputs
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtDoubleProperty),
                                     { typeId<DoubleData>() }
                                     )->addInPort = true;
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtStringProperty),
                                     { typeId<StringData>() }
                                     )->addInPort = true;
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtBoolProperty),
                                     { typeId<BoolData>() }
                                     )->addInPort = true;
        s_portFromPropertyMap.insert(GT_CLASSNAME(GtIntProperty),
                                     { typeId<IntData>() }
                                     )->addInPort = true;
        // forward changes made to objects
        auto iter =
            s_portFromPropertyMap.insert(GT_CLASSNAME(GtObjectLinkProperty),
                                         { typeId<ObjectData>() });
        iter->addInPort = true;
        iter->addOutPort = true;
        iter = s_portFromPropertyMap.insert(GT_CLASSNAME(GtObjectPathProperty),
                                            { typeId<ObjectData>() });
        iter->addInPort = true;
        iter->addOutPort = true;
    }

    /// Helper function to retrieve variant data for type `T`
    template<typename T>
    static inline QVariant
    variantFromNodeData(NodeDataPtr data, GtAbstractProperty&, GtCalculator&)
    {
        if (auto d = convert<T const>(data))
        {
            return d->value();
        }
        return {};
    }

    /// conversion functions that take a node data ptr and return a variant
    static inline void initVariantFromPortDataMap()
    {
        s_variantFromPortDataMap.insert(GT_CLASSNAME(DoubleData),
                                        variantFromNodeData<DoubleData>);
        s_variantFromPortDataMap.insert(GT_CLASSNAME(IntData),
                                        variantFromNodeData<IntData>);
        s_variantFromPortDataMap.insert(GT_CLASSNAME(BoolData),
                                        variantFromNodeData<BoolData>);
        s_variantFromPortDataMap.insert(GT_CLASSNAME(StringData),
                                        variantFromNodeData<StringData>);
        s_variantFromPortDataMap.insert(GT_CLASSNAME(ObjectData),
                                        [](NodeDataPtr data,
                                           GtAbstractProperty& prop,
                                           GtCalculator& calc) -> QVariant{
            auto d = convert<ObjectData const>(data);
            if (!d) return {};

            auto* o = d->object();
            auto* clone = o->clone();
            if (!clone) return {};

            if (qobject_cast<GtObjectLinkProperty*>(&prop))
            {
                calc.appendToLinkObjects(clone);
                return clone->uuid();
            }
            if (qobject_cast<GtObjectPathProperty*>(&prop))
            {
                calc.appendToLinkObjects(clone);
                GtObjectPath path(clone);
                return path.toString();
            }
            delete clone;
            return {};
        });
    }

    template<typename Value, typename T>
    static inline NodeDataPtr
    variantToNodeData(QVariant const& data, GtAbstractProperty&, GtCalculator&)
    {
        if (data.canConvert<Value>())
        {
            return std::make_shared<T>(data.value<Value>());
        }
        return {};
    }

    /// conversion functions that take a variant and return a node data ptr
    static inline void initVariantToPortDataMap()
    {
        s_variantToPortDataMap.insert(GT_CLASSNAME(DoubleData),
                                      variantToNodeData<double, DoubleData>);
        s_variantToPortDataMap.insert(GT_CLASSNAME(IntData),
                                      variantToNodeData<int, IntData>);
        s_variantToPortDataMap.insert(GT_CLASSNAME(StringData),
                                      variantToNodeData<QString, StringData>);
        s_variantToPortDataMap.insert(GT_CLASSNAME(BoolData),
                                      variantToNodeData<bool, BoolData>);
        s_variantToPortDataMap.insert(GT_CLASSNAME(ObjectData),
                                      [](QVariant const& data,
                                         GtAbstractProperty& prop,
                                         GtCalculator& calc) -> NodeDataPtr{
            if (auto* p = qobject_cast<GtObjectLinkProperty*>(&prop))
            {
                return std::make_shared<ObjectData>(calc.data(*p));
            }
            if (auto* p = qobject_cast<GtObjectPathProperty*>(&prop))
            {
                return std::make_shared<ObjectData>(calc.data(*p));
            }
            return {};
        });
    }

    static int inline init()
    {
        initPortFromPropertyMap();
        initVariantToPortDataMap();
        initVariantFromPortDataMap();
        return 0;
    }

}; // struct Impl

GenericCalculatorExecNode::GenericCalculatorExecNode() :
    Node(tr("Execute Calculator")),
    m_className("targetClassName",
                tr("Target class name"),
                tr("Target class name of calculator"))
{
    static auto init = Impl::init();
    Q_UNUSED(init);

    setNodeFlag(Resizable);
    m_outSuccess = addOutPort(PortInfo{typeId<BoolData>(), tr("success")});

    registerProperty(m_className);
    m_className.setReadOnly(true);

    connect(this, &Node::portConnected, this,
            &GenericCalculatorExecNode::onPortConnected);

    connect(this, &Node::portDisconnected, this,
            &GenericCalculatorExecNode::onPortDisconnected);

     registerWidgetFactory([this]() {
        auto w = std::make_unique<QWidget>();
        auto* lay = new QVBoxLayout;
        w->setLayout(lay);
        lay->setContentsMargins(0, 0, 0, 0);

        auto* edit = new QComboBox;
        edit->addItems(Impl::classIndents());
        edit->setStyleSheet(gt::gui::stylesheet::comboBox());

        lay->addWidget(edit);

        auto* view = new GtPropertyTreeView(gtApp->currentProject());
        view->setAnimated(false);
        lay->addWidget(view);

        auto updateView = [view, this](){
            view->setObject(nullptr);

            auto obj = this->currentObject();
            if (obj)
            {
                view->setObject(obj);
                // collapse first category
                view->collapse(view->model()->index(0, 0, view->rootIndex()));

                connect(obj, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                        this, &GenericCalculatorExecNode::onCurrentObjectDataChanged,
                        Qt::UniqueConnection);
            }
        };

        auto const updateClass = [this, edit](){
            m_className = Impl::identToClassName(edit->currentText());
        };
        auto const updateClassText = [this, edit](){
            edit->setCurrentText(Impl::classNameToIdent(m_className));
        };

        connect(edit, &QComboBox::currentTextChanged,
                this, updateClass);
        connect(&m_className, &GtAbstractProperty::changed,
                edit, updateClassText);
        connect(this, &GenericCalculatorExecNode::currentObjectChanged,
                view, updateView);

        /// iterate over ports to remove already connected ones at start
        for (auto* ports : {&m_calcInPorts, &m_calcOutPorts})
        {
            gt::for_each_key(ports->begin(), ports->end(),
                             [this](PortId portId){
                if (nodeData<NodeData const>(portId))
                {
                    onPortConnected(portId);
                }
            });
        }

        m_className.get().isEmpty() ? updateClass() : updateClassText();
        updateView();

        return w;
    });

    connect(&m_className, &GtAbstractProperty::changed,
            this, &GenericCalculatorExecNode::updateCurrentObject);
}

void
GenericCalculatorExecNode::eval()
{
    bool success = false;

    // set output data depending on success
    auto finally = gt::finally([&success, this](){
        setNodeData(m_outSuccess, std::make_shared<BoolData>(success));
    });

    auto obj = currentObject();
    if (!obj) return;

    auto* calc = qobject_cast<GtCalculator*>(obj);
    if (!calc) return;

    // linked objects should be empty
    auto linkedObjects = calc->linkedObjects();
    assert(linkedObjects.empty());

    // apply input data to calculator
    gt::for_each_key(m_calcInPorts.begin(), m_calcInPorts.end(),
                     [this, calc](PortId portId){
         assert(portType(portId) == PortType::In);

         auto* port = this->port(portId);
         if (!port) return;

         auto prop = calc->findProperty(m_calcInPorts[portId]);
         if (!prop) return;

         prop->setActive(true);

         auto getVariant = Impl::variantFromPortData(port->typeId);
         if (!getVariant) return;

         QVariant value = getVariant(nodeData(portId), *prop, *calc);

         gtTrace() << tr("Input port %1: setting property '%2' =")
                          .arg(portId)
                          .arg(prop->ident()) << value;

         prop->setValueFromVariant(value);
    });

    // linked objects may have updated
    linkedObjects = calc->linkedObjects();

    // execute calculator
    gtTrace() << tr("Executing calculator '%1'...").arg(calc->objectName());
    success = calc->run();
    gtTrace() << tr("Execution was %1successfull!").arg(success ? "":"un");

    // extract output data from calculator
    gt::for_each_key(m_calcOutPorts.begin(), m_calcOutPorts.end(),
                     [this, calc](PortId portId){
        assert(portType(portId) == PortType::Out);

        auto* port = this->port(portId);
        if (!port) return;

        auto prop = calc->findProperty(m_calcOutPorts[portId]);
        if (!prop) return;

        auto getNodeData = Impl::variantToPortData(port->typeId);
        if (!getNodeData) return;

        bool success = true;
        QVariant value = prop->valueToVariant({}, &success);

        NodeDataPtr data = success ?
                               getNodeData(value, *prop, *calc) :
                               nullptr;

        gtTrace() << tr("Output port %1: setting data from property '%2'")
                         .arg(portId)
                         .arg(prop->ident()) << data;

        setNodeData(portId, std::move(data));
    });

    // delete linked objects
    qDeleteAll(linkedObjects);
}


GtObject*
GenericCalculatorExecNode::currentObject()
{
    return findDirectChild<GtObject*>();
}

void
GenericCalculatorExecNode::initPorts() // generate default parameter set
{
    clearPorts();

    if (!Impl::isKnownClass(m_className)) return;

    GtObject* object = currentObject();
    if (!object) return;

    // dynamic in ports for gt_object properties
    auto const& properties =  object->fullPropertyList();
    if (properties.empty()) return;

    // dynamically extract default category
    auto const& defaultCategory = properties.at(0)->categoryToString();

    for (GtAbstractProperty* prop : properties)
    {
        // do not append hidden properties
        if (prop->isHidden()) continue;

        // do not add ports for default entries of process elements
        if (prop->category() == GtAbstractProperty::Custom &&
            prop->categoryToString() == defaultCategory)
        {
            // hide some default entries
            if (prop->ident() == QStringLiteral("skip")) prop->hide(true);
            continue;
        }

        QString const& ident = prop->ident();

        auto const containsPort = [&ident](auto const& ports){
            return std::any_of(ports.begin(), ports.end(),
                               [&ident](QString const& portIdent){
                return ident == portIdent;
            });
        };

        if (containsPort(m_calcInPorts) || containsPort(m_calcOutPorts)) continue;

        char const* className = prop->metaObject()->className();

        // add port depending on the property type
        auto entry = Impl::propertyToPortType(className);
        if (!entry) continue;

        PortInfo port = entry.typeId;
        port.caption = Impl::generatePortCaption(*prop);
        port.captionVisible = true;
        port.optional = true;

#if GT_VERSION >= GT_VERSION_CHECK(2, 1, 0)
        if (prop->isMonitoring())
        {
            entry.addInPort = false;
            entry.addOutPort = true;
        }
#endif

        if (entry.addInPort)
        {
            PortId portId = addInPort(port);
            if (portId != invalid<PortId>()) m_calcInPorts.insert(portId, ident);
        }
        if (entry.addOutPort)
        {
            PortId portId = addOutPort(port);
            if (portId != invalid<PortId>()) m_calcOutPorts.insert(portId, ident);
        }
    }
}

void
GenericCalculatorExecNode::clearPorts()
{
    auto const doClearPorts = [this](auto& ports){
        gt::for_each_key(ports.cbegin(), ports.cend(), [this](PortId key){
            removePort(key);
        });
        ports.clear();
    };

    doClearPorts(m_calcInPorts);
    doClearPorts(m_calcOutPorts);
}

void
GenericCalculatorExecNode::updateCurrentObject()
{
    QString const& classText = m_className.getVal();

    GtObject* obj = currentObject();

    // currentObject Class
    if (obj)
    {
        char const* oldClassName = obj->metaObject()->className();

        if (classText == oldClassName)
        {
            gtTrace() << tr("Class name not changed. Keep '%1'")
                             .arg(oldClassName);
            return;
        }
    }

    if (!Impl::isKnownClass(classText))
    {
        gtTrace() << tr("ClassName '%1' not known").arg(classText);

        if (obj)
        {
            char const* newClassName = obj->metaObject()->className();

            gtTrace() << tr("Reset to valid class name '%1'")
                             .arg(newClassName);

            m_className.setVal(newClassName);
        }

        return;
    }

    delete obj;

    clearPorts();

    std::unique_ptr<GtObject> o{gtProcessFactory->newObject(m_className)};

    if (o)
    {
        o->setDefault(true);
        o->setUserHidden(true);

        if (appendChild(o.get()))
        {
            o.release();
        }
    }

    initPorts();

    emit currentObjectChanged();

    emit triggerNodeEvaluation();
}

void
GenericCalculatorExecNode::onCurrentObjectDataChanged()
{
    emit nodeChanged();
    emit triggerNodeEvaluation();
}

void
GenericCalculatorExecNode::onPortConnected(PortId portId)
{
    bool success = Impl::setPortPropertyHidden(*this, m_calcInPorts, portId, true);
    if (!success)
    {
        Impl::setPortPropertyHidden(*this, m_calcOutPorts, portId, true);
    }
}

void
GenericCalculatorExecNode::onPortDisconnected(PortId portId)
{
    bool success = Impl::setPortPropertyHidden(*this, m_calcInPorts, portId, false);
    if (!success)
    {
        Impl::setPortPropertyHidden(*this, m_calcOutPorts, portId, false);
    }
}

bool
GenericCalculatorExecNode::addToWhiteList(QStringList const& whiteList)
{
    if (whiteList.isEmpty()) return false;

    QStringList const& knownClasses = gtProcessFactory->knownClasses();

    for (ClassName const& c : knownClasses)
    {
        if (!whiteList.contains(c)) continue;

        if (s_knownClasses.contains(c)) continue;

        GtCalculatorData const& d = gtCalculatorFactory->calculatorData(c);

        if (d) s_knownClasses.insert(c, d->id);
    }

    return true;
}
