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

#include "intelli/node/general/tolist.h"
#include "intelli/node/general/listgetitem.h"
#include "intelli/node/general/listgetsize.h"

#include "intelli/node/input/boolinput.h"
#include "intelli/node/input/doubleinput.h"
#include "intelli/node/input/fileinput.h"
#include "intelli/node/input/intinput.h"
#include "intelli/node/input/objectinput.h"
#include "intelli/node/input/stringinput.h"
#include "intelli/node/input/graphuservariablesinput.h"

#include "intelli/node/booldisplay.h"
#include "intelli/node/textdisplay.h"

#include "intelli/node/genericcalculatorexec.h"
#include "intelli/node/filereader.h"
#include "intelli/node/filewriter.h"
#include "intelli/node/stringselection.h"

#include "intelli/node/projectinfo.h"

#include "intelli/node/objectsink.h"

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

        QString catInput   = QObject::tr("Input");
        QString catNumber  = QObject::tr("Number");
        QString catDisplay = QObject::tr("Display");
        QString catString  = QObject::tr("String");
        QString catObject  = QObject::tr("Object");
        QString catGeneral = QObject::tr("General");
        QString catProcess = QObject::tr("Process");
        QString catFile    = QObject::tr("File");
        QString catLogic   = QObject::tr("Logic");
        QString catOther   = QObject::tr("Other");
        View<QString> hidden{};

        auto& factory = NodeFactory::instance();
        factory.setCategoryOrderPriority(catInput, -100);
        factory.setCategoryOrderPriority(catDisplay, -80);
        factory.setCategoryOrderPriority(catNumber, -60);
        factory.setCategoryOrderPriority(catString, -40);
        factory.setCategoryOrderPriority(catObject, -20);
        factory.setCategoryOrderPriority(catGeneral, -10);
        factory.setCategoryOrderPriority(catProcess, 20);
        factory.setCategoryOrderPriority(catFile,  40);
        factory.setCategoryOrderPriority(catLogic, CategoryOrderPriority::Last);
        factory.setCategoryOrderPriority(catOther, CategoryOrderPriority::Last);

        factory.registerNode<DummyNode>(hidden);

        factory.registerNode<Graph>(hidden);
        factory.registerNode<GroupInputProvider>(hidden);
        factory.registerNode<GroupOutputProvider>(hidden);

        factory.registerNode<SleepyNode>((gtApp && gtApp->devMode()) ?
                                             catOther : QString{});

        factory.registerNode<IntInputNode>({catNumber, catInput});
        factory.registerNode<DoubleInputNode>({catNumber, catInput});
        factory.registerNode<NumberDisplayNode>({catNumber, catDisplay});
        factory.registerNode<NumberMathNode>({catNumber});

        factory.registerNode<BoolInputNode>({catLogic, catInput});
        factory.registerNode<BoolDisplayNode>({catLogic, catDisplay});
        factory.registerNode<LogicNode>({catLogic});

        factory.registerNode<StringInputNode>({catString, catInput});
        factory.registerNode<TextDisplayNode>({catString, catDisplay});
        factory.registerNode<StringBuilderNode>({catString});
        factory.registerNode<StringSelectionNode>({catString});

        factory.registerNode<ObjectInputNode>({catObject, catInput});
        factory.registerNode<ObjectMementoNode>({catObject});
        factory.registerNode<FindDirectChildNode>({catObject});
        factory.registerNode<ObjectSink>({catObject});

        factory.registerNode<FileInputNode>({catInput, catFile});
        factory.registerNode<FileReaderNode>({catFile});
        factory.registerNode<FileWriterNode>({catFile});
        factory.registerNode<ExistingDirectorySourceNode>({catInput, catFile});

        factory.registerNode<ProjectInfoNode>({catInput});
        factory.registerNode<GraphUserVariablesInputNode>({catInput});

        factory.registerNode<GenericCalculatorExecNode>({catProcess});

        factory.registerNode<ToListNode>({catGeneral});
        factory.registerNode<ListGetItemNode>({catGeneral});
        factory.registerNode<ListGetSizeNode>({catGeneral});

#ifdef GT_INTELLI_EXPERIMENTAL_NODES
        factory.registerNode<BinaryDisplayNode>({catLogic, catDisplay});
#endif
        return true;
    }();

    Q_UNUSED(_);
}
