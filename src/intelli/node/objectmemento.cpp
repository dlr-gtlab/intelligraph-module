#include <intelli/node/objectmemento.h>

#include <gt_objectmemento.h>

#include <intelli/data/object.h>
#include <intelli/data/string.h>

#include <QLayout>

using namespace intelli;

ObjectMementoNode::ObjectMementoNode() :
    Node("To Memento")
{
    m_in  = addInPort(typeId<ObjectData>());
    m_out = addOutPort({typeId<StringData>(), tr("memento")});
}

void
ObjectMementoNode::eval()
{
    auto data = nodeData<ObjectData>(m_in);

    if (!data || !data->object())
    {
        setNodeData(m_out, nullptr);
        return;
    }

    setNodeData(m_out, std::make_shared<StringData>(data->object()->toMemento().toByteArray()));
}
