#include "gt_object.h"
#include "gt_objectmemento.h"
#include "gt_xmlhighlighter.h"

#include "gtd_predesignplot3d.h"

#include "gt_codeeditor.h"

#include "nds_objectdata.h"

#include "nds_objectmementomodel.h"

NdsObjectMementoModel::NdsObjectMementoModel() :
    m_editor(new GtCodeEditor)
{
    m_editor->setMinimumSize(300, 300);

    m_editor->setReadOnly(true);
    new GtXmlHighlighter(m_editor->document());
}

QWidget*
NdsObjectMementoModel::embeddedWidget()
{
    return m_editor;
}

unsigned int
NdsObjectMementoModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 1;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsObjectMementoModel::dataType(PortType const, PortIndex const) const
{
    return NdsObjectData().type();
}

std::shared_ptr<NodeData>
NdsObjectMementoModel::outData(PortIndex)
{
    return _nodeData;
}

void
NdsObjectMementoModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
    _nodeData = nodeData;

    if (_nodeData)
    {
        auto d = std::dynamic_pointer_cast<NdsObjectData>(_nodeData);

        m_editor->clear();

        if (d->object())
        {
            GtObjectMemento mem = d->object()->toMemento();
            m_editor->setPlainText(mem.toByteArray());
        }
    }
    else
    {
        m_editor->clear();
    }

    Q_EMIT dataUpdated(0);
}
