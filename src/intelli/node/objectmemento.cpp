#include "intelli/node/objectmemento.h"
#include "intelli/nodefactory.h"

#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"
#include "gt_codeeditor.h"

#include <intelli/data/object.h>

#include <QLayout>

using namespace intelli;

static auto init_once = [](){
    return GT_INTELLI_REGISTER_NODE(ObjectMementoNode, "Object")
}();

ObjectMementoNode::ObjectMementoNode() :
    Node(tr("Memento Viewer"))
{
    setNodeFlag(Resizable);

    PortId inPort = addInPort(typeId<ObjectData>());

    registerWidgetFactory([this, inPort]() {
        auto base = makeWidget();
        auto* w = new GtCodeEditor();
        base->layout()->addWidget(w);

        w->setMinimumSize(300, 300);
        w->setReadOnly(true);
        new GtXmlHighlighter(w->document());

        auto const update = [this, inPort, w](){
            w->clear();
            if (auto* data = nodeData<ObjectData*>(inPort))
            {
                if (auto* obj = data->object())
                {
                    w->setPlainText(obj->toMemento().toByteArray());
                }
            }
        };
        
        connect(this, &Node::evaluated, w, update);
        update();

        return base;
    });
}
