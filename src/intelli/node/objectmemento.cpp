#include "intelli/node/objectmemento.h"
#include "intelli/nodefactory.h"

#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"
#include "gt_codeeditor.h"

#include "intelli/data/object.h"

using namespace intelli;

GT_INTELLI_REGISTER_NODE(ObjectMementoNode, "Object");

ObjectMementoNode::ObjectMementoNode() :
    Node(tr("Memento Viewer"))
{
    setNodeFlag(Resizable);
    
    PortId inPort = addInPort(typeId<ObjectData>());

    registerWidgetFactory([=]() {
        auto w = std::make_unique<GtCodeEditor>();
        w->setMinimumSize(300, 300);
        w->setReadOnly(true);
        new GtXmlHighlighter(w->document());

        auto const update = [=, w_ = w.get()](){
            w_->clear();
            if (auto* data = nodeData<ObjectData*>(inPort))
            {
                if (auto* obj = data->object())
                {
                    w_->setPlainText(obj->toMemento().toByteArray());
                }
            }
        };
        
        connect(this, &Node::inputDataRecieved, w.get(), update);
        update();

        return w;
    });
}
