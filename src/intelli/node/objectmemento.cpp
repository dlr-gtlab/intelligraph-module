#include "intelli/node/objectmemento.h"

#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"
#include "gt_codeeditor.h"

#include <intelli/data/object.h>

#include <QLayout>

using namespace intelli;

ObjectMementoNode::ObjectMementoNode() :
    Node(tr("Memento Viewer"))
{
    setNodeFlag(Resizable);
    setNodeEvalMode(NodeEvalMode::MainThread);

    PortId inPort = addInPort(typeId<ObjectData>());

    registerWidgetFactory([this, inPort]() {
        auto base = makeBaseWidget();
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

        connect(this, &Node::inputDataRecieved, w, update);
        update();

        return base;
    });
}
