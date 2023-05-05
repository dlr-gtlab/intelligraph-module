#include "gt_igobjectmementonode.h"

#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"

#include "gt_codeeditor.h"

#include "models/data/gt_igobjectdata.h"

GTIG_REGISTER_NODE(GtIgObjectMementoNode);

GtIgObjectMementoNode::GtIgObjectMementoNode() :
    GtIntelliGraphNode(tr("Memento Viewer"))
{
    setNodeFlag(gt::ig::Resizable);
}

unsigned int
GtIgObjectMementoNode::nPorts(PortType type) const
{
    switch (type)
    {
    case PortType::In:
    case PortType::Out:
        return 1;
    case PortType::None:
        return 0;
    }
    throw std::logic_error{"Unhandled enum value!"};

}

GtIgObjectMementoNode::NodeDataType
GtIgObjectMementoNode::dataType(PortType const type,
                                 PortIndex const idx) const
{
    return GtIgObjectData::staticType();
}

GtIgObjectMementoNode::NodeData
GtIgObjectMementoNode::outData(PortIndex)
{
    return m_data;
}

void
GtIgObjectMementoNode::setInData(NodeData data, PortIndex const)
{
    if (!m_editor) initWidget();
    m_data = gt::ig::nodedata_cast<GtIgObjectData>(std::move(data));

    m_editor->clear();

    if (m_data && m_data->object())
    {
        GtObjectMemento mem = m_data->object()->toMemento();
        m_editor->setPlainText(mem.toByteArray());
    }
}

QWidget*
GtIgObjectMementoNode::embeddedWidget()
{
    if (!m_editor) initWidget();
    return m_editor;
}

void
GtIgObjectMementoNode::initWidget()
{
    m_editor = gt::ig::make_volatile<GtCodeEditor>();
    m_editor->setMinimumSize(300, 300);
    m_editor->setReadOnly(true);
    new GtXmlHighlighter(m_editor->document());
}
