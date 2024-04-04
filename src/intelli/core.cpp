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

#include "intelli/nodedatafactory.h"
#include "intelli/nodefactory.h"

using namespace intelli;

void
intelli::initModule()
{
    static auto _ = [](){
        gtTrace() << "Initializing nodes...";

        GT_INTELLI_REGISTER_DATA(BoolData);
        GT_INTELLI_REGISTER_DATA(ByteArrayData);
        GT_INTELLI_REGISTER_DATA(DoubleData);
        GT_INTELLI_REGISTER_DATA(IntData);
        GT_INTELLI_REGISTER_DATA(IntegerData);
        GT_INTELLI_REGISTER_DATA(ObjectData);
        GT_INTELLI_REGISTER_DATA(StringData);
        GT_INTELLI_REGISTER_DATA(StringListData);

        GT_INTELLI_REGISTER_NODE(Graph, "Other")
        GT_INTELLI_REGISTER_NODE(GroupInputProvider, "")
        GT_INTELLI_REGISTER_NODE(GroupOutputProvider, "")
        GT_INTELLI_REGISTER_NODE(NumberDisplayNode, "Number");
        GT_INTELLI_REGISTER_NODE(NumberMathNode, "Number");
        GT_INTELLI_REGISTER_NODE(NumberSourceNode, "Number")
        GT_INTELLI_REGISTER_NODE(SleepyNode, "Number");

        GT_INTELLI_REGISTER_NODE(LogicDisplayNode, "Logic");
        GT_INTELLI_REGISTER_NODE(LogicNode, "Logic");
        GT_INTELLI_REGISTER_NODE(LogicSourceNode, "Logic");

        GT_INTELLI_REGISTER_NODE(ObjectMementoNode, "Object");
        GT_INTELLI_REGISTER_NODE(ObjectSourceNode, "Object");
        GT_INTELLI_REGISTER_NODE(FindDirectChildNode, "Object")

        GT_INTELLI_REGISTER_NODE(StringListInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(ExistingDirectorySourceNode, "Input");


        GT_INTELLI_REGISTER_NODE(BoolInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(DoubleInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(StringInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(ObjectInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(IntInputNode, "Input");
        GT_INTELLI_REGISTER_NODE(IntInputNode, "Input");


        return true;
    }();

    Q_UNUSED(_);
}
