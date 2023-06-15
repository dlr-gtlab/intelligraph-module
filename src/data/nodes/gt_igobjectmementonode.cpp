#include "gt_igobjectmementonode.h"
#include "gt_intelligraphnodefactory.h"

#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"
#include "gt_codeeditor.h"

#include "gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgObjectMementoNode, "Object");

GtIgObjectMementoNode::GtIgObjectMementoNode() :
    GtIntelliGraphNode(tr("Memento Viewer"))
{
    setNodeFlag(gt::ig::Resizable);

    PortId inPort = addInPort(gt::ig::typeId<GtIgObjectData>());

    registerWidgetFactory([=](GtIntelliGraphNode&) {
        auto w = std::make_unique<GtCodeEditor>();
        w->setMinimumSize(300, 300);
        w->setReadOnly(true);
        new GtXmlHighlighter(w->document());
        
        connect(this, &GtIntelliGraphNode::inputDataRecieved, w.get(), [=, w_ = w.get()](){
            w_->clear();
            if (auto* data = qobject_cast<GtIgObjectData const*>(portData(inPort)))
            {
                if (auto* obj = data->object())
                {
                    w_->setPlainText(obj->toMemento().toByteArray());
                }
            }
        });

        return w;
    });
}
