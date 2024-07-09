#include "intelli/core.h"

#include "intelli/data/bool.h"
#include "intelli/data/double.h"
#include "intelli/data/object.h"
#include "intelli/data/stringlist.h"
#include "intelli/data/string.h"
#include "intelli/data/integer.h"
#include "intelli/data/bytearraydata.h"

#include "intelli/graph.h"
#include "intelli/node/groupinputprovider.h"
#include "intelli/node/groupoutputprovider.h"

#include "intelli/node/numbersource.h"
#include "intelli/node/numberdisplay.h"
#include "intelli/node/numbermath.h"
#include "intelli/node/sleepy.h"

#include "intelli/node/objectmemento.h"
#include "intelli/node/objectsource.h"
#include "intelli/node/stringlistinput.h"
#include "intelli/node/finddirectchild.h"
#include "intelli/node/existingdirectorysource.h"

#include "intelli/node/logicdisplay.h"
#include "intelli/node/logicoperation.h"
#include "intelli/node/logicsource.h"

#include "intelli/node/propertyinput/boolinputnode.h"
#include "intelli/node/propertyinput/doubleinputnode.h"
#include "intelli/node/propertyinput/intinputnode.h"
#include "intelli/node/propertyinput/objectinputnode.h"
#include "intelli/node/propertyinput/stringinputnode.h"

#include "intelli/node/genericcalculatorexec.h"

#include "intelli/nodedatafactory.h"
#include "intelli/nodefactory.h"

using namespace intelli;

void
intelli::initModule()
{
    static auto _ = [](){
        gtTrace().verbose() << QObject::tr("Initializing nodes...");

        GT_INTELLI_REGISTER_DATA(BoolData);
        GT_INTELLI_REGISTER_DATA(ByteArrayData);
        GT_INTELLI_REGISTER_DATA(DoubleData);
        GT_INTELLI_REGISTER_DATA(IntData);
        GT_INTELLI_REGISTER_DATA(ObjectData);
        GT_INTELLI_REGISTER_DATA(StringData);
        GT_INTELLI_REGISTER_DATA(StringListData);

        char const* hidden = "";
        QString catOther = QObject::tr("Other");
        QString catNumber = QObject::tr("Number");
        QString catLogic = QObject::tr("Logic");
        QString catObject = QObject::tr("Object");
        QString catInput = QObject::tr("Input");
        QString catProcess = QObject::tr("Process");

        GT_INTELLI_REGISTER_NODE(Graph, catOther)
        GT_INTELLI_REGISTER_NODE(GroupInputProvider, hidden)
        GT_INTELLI_REGISTER_NODE(GroupOutputProvider, hidden)
        GT_INTELLI_REGISTER_NODE(NumberDisplayNode, catNumber);
        GT_INTELLI_REGISTER_NODE(NumberMathNode, catNumber);
        GT_INTELLI_REGISTER_NODE(NumberSourceNode, catNumber)
        GT_INTELLI_REGISTER_NODE(SleepyNode, catNumber);

        GT_INTELLI_REGISTER_NODE(LogicDisplayNode, catLogic);
        GT_INTELLI_REGISTER_NODE(LogicNode, catLogic);
        GT_INTELLI_REGISTER_NODE(LogicSourceNode, catLogic);

        GT_INTELLI_REGISTER_NODE(ObjectMementoNode, catObject);
        GT_INTELLI_REGISTER_NODE(ObjectSourceNode, catObject);
        GT_INTELLI_REGISTER_NODE(FindDirectChildNode, catObject)

        GT_INTELLI_REGISTER_NODE(StringListInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(ExistingDirectorySourceNode, catInput);
        GT_INTELLI_REGISTER_NODE(BoolInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(DoubleInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(StringInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(ObjectInputNode, catInput);
        GT_INTELLI_REGISTER_NODE(IntInputNode, catInput);

        GT_INTELLI_REGISTER_NODE(GenericCalculatorExecNode, catProcess);

        return true;
    }();

    Q_UNUSED(_);
}
