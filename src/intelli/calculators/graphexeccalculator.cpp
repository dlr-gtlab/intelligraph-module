/* GTlab - Gas Turbine laboratory
 * Source File:
 * copyright 2009-2023 by DLR
 *
 *  Created on: 06.03.2024
 *  Author: Jens Schmeink (AT-TWK)
 *  Tel.: +49 2203 601 2191
 */
#include "graphexeccalculator.h"
#include <QRegExpValidator>

#include <intelli/graph.h>
#include <intelli/graphexecmodel.h>

#include <intelli/node/propertyinput/boolinputnode.h>
#include <intelli/node/propertyinput/intinputnode.h>
#include <intelli/node/propertyinput/doubleinputnode.h>
#include <intelli/node/propertyinput/stringinputnode.h>

#include "gt_regexp.h"
#include "gt_structproperty.h"
#include "gt_doubleproperty.h"
#include "gt_intproperty.h"
#include "gt_stringproperty.h"
#include "gt_boolproperty.h"


namespace intelli

{
GraphExecCalculator::GraphExecCalculator() :
    m_intelli("intelli", tr("IntelliGraph"),
                tr("Link to IntelliGraph"), "",
                this, QStringList() << GT_CLASSNAME(intelli::Graph)),
    m_numberNodeContainer("propertyNodes", "Property Nodes")//,
{

    setObjectName("Graph Execution");
    registerProperty(m_intelli);

    auto makeStringWithEmptySpace = [](QString const& id)
    {
        auto* val = new QRegExpValidator(gt::re::onlyLettersAndNumbersAndSpace());
        auto* stringProp = new GtStringProperty(id, QObject::tr("NodeName"),
                                              QObject::tr("NodeName"), "",
                                              val);
        return stringProp;
    };

    GtPropertyStructDefinition doubleNodes("Double Node");
    doubleNodes.defineMember("NodeName", makeStringWithEmptySpace);
    doubleNodes.defineMember("Value", gt::makeDoubleProperty(0));
    m_numberNodeContainer.registerAllowedType(doubleNodes);

    GtPropertyStructDefinition intNodes("Int Node");
    intNodes.defineMember("NodeName", makeStringWithEmptySpace);
    intNodes.defineMember("Value", gt::makeIntProperty(0));
    m_numberNodeContainer.registerAllowedType(intNodes);

    GtPropertyStructDefinition boolNodes("Bool Node");
    boolNodes.defineMember("NodeName", makeStringWithEmptySpace);
    boolNodes.defineMember("Value", gt::makeBoolProperty(0));
    m_numberNodeContainer.registerAllowedType(boolNodes);

    GtPropertyStructDefinition stringNodes("String Node");
    stringNodes.defineMember("NodeName", makeStringWithEmptySpace);
    stringNodes.defineMember("Value", gt::makeStringProperty(0));
    m_numberNodeContainer.registerAllowedType(stringNodes);

    registerPropertyStructContainer(m_numberNodeContainer);
}

bool
GraphExecCalculator::run()
{
    auto* graph1 = data<intelli::Graph*>(m_intelli);

    if (!graph1)
    {
        gtError() << tr("IntelliGraph not found!");
        return false;
    }

    GtObject* helper = graph1->copy();

    intelli::Graph* graph = qobject_cast<intelli::Graph*>(helper);

    for (auto& prop : m_numberNodeContainer)
    {
        QString name = prop.getMemberVal<QString>("NodeName");

        bool found = false;

        if (prop.typeName() == "Double Node")
        {
            double val = prop.getMemberVal<double>("Value");
            auto* n = graph->findDirectChild<DoubleInputNode*>(name);

            if (n)
            {
                n->setValue(val);
                found = true;
                gtTrace() << tr("Set node '%1' to '%2'").arg(
                                 name, QString::number(val));
            }
        }
        else if (prop.typeName() == "Int Node")
        {
            int val = prop.getMemberVal<int>("Value");
            auto* n = graph->findDirectChild<IntInputNode*>(name);

            if (n)
            {
                n->setValue(val);
                found = true;
                gtTrace() << tr("Set node '%1' to '%2'").arg(
                                 name, QString::number(val));
            }
        }
        else if(prop.typeName() == "Bool Node")
        {
            bool val = prop.getMemberVal<bool>("Value");
            auto* n = graph->findDirectChild<BoolInputNode*>(name);

            if (n)
            {
                n->setValue(val);
                found = true;
                gtTrace() << tr("Set node '%1' to '%2'").arg(name, val);
            }
        }
        else if(prop.typeName() == "String Node")
        {
            QString val = prop.getMemberVal<QString>("Value");
            auto* n = graph->findDirectChild<StringInputNode*>(name);

            if (n)
            {
                n->setValue(val);
                found = true;
                gtTrace() << tr("Set node '%1' to '%2'").arg(name, val);
            }
        }
        else
        {
            gtError() << tr("Found a property with node name '%1' which could "
                            "not be used").arg(name);
            return false;
        }

        if (!found)
        {
            gtWarning() << tr("Cannot find node '%1' to "
                              "set value for.").arg(name);
        }
    }

    intelli::GraphExecutionModel model(*graph);
    model.reset();

    bool success = model.evaluateGraph().wait();

    gtFatal() << "Test 1";
    graph->deleteLater();
    gtFatal() << "Test 2";

    /// ToDo:
    /// Add monitoring information if possible

    return success;
}

}
