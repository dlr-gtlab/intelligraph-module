/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marting Siggel <martin.siggel@dlr.de>
 */

#include "intelli/core.h"

#include "intelli/data/bool.h"
#include "intelli/data/bytearray.h"
#include "intelli/data/double.h"
#include "intelli/data/object.h"
#include "intelli/data/string.h"
#include "intelli/data/int.h"
#include "intelli/data/file.h"
#include "intelli/data/stringlist.h"

#include "intelli/node/dummy.h"

#include "intelli/graph.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include "intelli/node/numberdisplay.h"
#include "intelli/node/numbermath.h"
#include "intelli/node/sleepy.h"

#include "intelli/node/objectmemento.h"
#include "intelli/node/finddirectchild.h"
#include "intelli/node/existingdirectorysource.h"

#include "intelli/node/logicoperation.h"

#include "intelli/node/stringbuilder.h"

#include "intelli/node/input/boolinput.h"
#include "intelli/node/input/doubleinput.h"
#include "intelli/node/input/fileinput.h"
#include "intelli/node/input/intinput.h"
#include "intelli/node/input/objectinput.h"
#include "intelli/node/input/stringinput.h"

#include "intelli/node/booldisplay.h"
#include "intelli/node/textdisplay.h"

#include "intelli/node/genericcalculatorexec.h"
#include "intelli/node/filereader.h"
#include "intelli/node/filewriter.h"
#include "intelli/node/stringselection.h"

#include "intelli/node/projectinfo.h"

#include "intelli/nodedatafactory.h"
#include "intelli/nodefactory.h"

#ifdef GT_INTELLI_EXPERIMENTAL_NODES
#include "intelli/node/binarydisplay.h"
#endif

using namespace intelli;


void
intelli::initModule()
{
    registerDefaultDataTypes();
    registerDefaultNodes();
}

void
intelli::registerDefaultDataTypes()
{
    static auto _ = [](){
        gtTrace().verbose() << QObject::tr("Registering default data types...");

        // register data type
        GT_INTELLI_REGISTER_DATA(ByteArrayData);
        GT_INTELLI_REGISTER_DATA(StringData);
        GT_INTELLI_REGISTER_DATA(StringListData);
        GT_INTELLI_REGISTER_DATA(DoubleData);
        GT_INTELLI_REGISTER_DATA(IntData);
        GT_INTELLI_REGISTER_DATA(BoolData);
        GT_INTELLI_REGISTER_DATA(ObjectData);
        GT_INTELLI_REGISTER_DATA(FileData);

        // register conversions
        gtTrace().verbose() << QObject::tr("Registering default conversions...");

        GT_INTELLI_REGISTER_INLINE_CONVERSION(StringData, ByteArrayData, data->value().toUtf8());
        GT_INTELLI_REGISTER_INLINE_CONVERSION(ByteArrayData, StringData, data->value());

        GT_INTELLI_REGISTER_INLINE_CONVERSION(DoubleData, IntData, data->value());
        GT_INTELLI_REGISTER_INLINE_CONVERSION(IntData, DoubleData, data->value());

        return true;
    }();

    Q_UNUSED(_);
}

void
intelli::registerDefaultNodes()
{
    static auto _ = [](){
        gtTrace().verbose() << QObject::tr("Registering default nodes...");

        char const* hidden = "";
        QString catOther = QObject::tr("Other");
        QString catNumber = QObject::tr("Number");
        QString catLogic = QObject::tr("Logic");
        QString catObject = QObject::tr("Object");
        QString catString = QObject::tr("String");
        QString catInput = QObject::tr("Input");
        QString catProcess = QObject::tr("Process");
        QString catFile = QObject::tr("File");
        QString catDisplay = QObject::tr("Display");

        GT_INTELLI_REGISTER_NODE(DummyNode, hidden);

        GT_INTELLI_REGISTER_NODE(Graph, catOther);
        GT_INTELLI_REGISTER_NODE(GroupInputProvider, hidden);
        GT_INTELLI_REGISTER_NODE(GroupOutputProvider, hidden);

        GT_INTELLI_REGISTER_NODE(NumberDisplayNode, catDisplay);
        GT_INTELLI_REGISTER_NODE(NumberMathNode, catNumber);
        GT_INTELLI_REGISTER_NODE(SleepyNode, (gtApp && gtApp->devMode()) ? catOther : hidden);
        GT_INTELLI_REGISTER_NODE(BoolDisplayNode, catDisplay);

        GT_INTELLI_REGISTER_NODE(LogicNode, catLogic);

        GT_INTELLI_REGISTER_NODE(TextDisplayNode, catDisplay);

        GT_INTELLI_REGISTER_NODE(ObjectMementoNode, catObject);
        GT_INTELLI_REGISTER_NODE(FindDirectChildNode, catObject);

        GT_INTELLI_REGISTER_NODE(FileInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(FileReaderNode, catFile);
        GT_INTELLI_REGISTER_NODE(FileWriterNode, catFile);

        GT_INTELLI_REGISTER_NODE(ProjectInfoNode, catInput);

        GT_INTELLI_REGISTER_NODE(ExistingDirectorySourceNode, catInput);
        GT_INTELLI_REGISTER_NODE(BoolInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(DoubleInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(StringInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(ObjectInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(IntInputNode, catInput);

        GT_INTELLI_REGISTER_NODE(StringBuilderNode, catString);
        GT_INTELLI_REGISTER_NODE(StringSelectionNode, catString);

        GT_INTELLI_REGISTER_NODE(GenericCalculatorExecNode, catProcess);

#ifdef GT_INTELLI_EXPERIMENTAL_NODES
        GT_INTELLI_REGISTER_NODE(BinaryDisplayNode, catDisplay);
#endif
        return true;
    }();

    Q_UNUSED(_);
}
