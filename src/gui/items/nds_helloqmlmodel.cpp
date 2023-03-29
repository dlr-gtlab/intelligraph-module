#include <QQuickWidget>

#include "nds_objectdata.h"

#include "nds_helloqmlmodel.h"

NdsHelloQmlModel::NdsHelloQmlModel() :
    m_qmlWid(new QQuickWidget)
{
//    m_qmlWid->setSource(QUrl("qrc:/qml/main.qml"));
//    m_qmlWid->setResizeMode(QQuickWidget::SizeRootObjectToView);
}

QWidget*
NdsHelloQmlModel::embeddedWidget()
{

    return m_qmlWid;
}

unsigned int
NdsHelloQmlModel::nPorts(PortType portType) const
{
    unsigned int result = 1;

    switch (portType) {
    case PortType::In:
        result = 1;
        break;

    case PortType::Out:
        result = 0;

    default:
        break;
    }

    return result;
}

NodeDataType
NdsHelloQmlModel::dataType(PortType const, PortIndex const) const
{
    return NdsObjectData().type();
}

std::shared_ptr<NodeData>
NdsHelloQmlModel::outData(PortIndex)
{
    return {};
}

void
NdsHelloQmlModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex const)
{
//    _nodeData = nodeData;

//    if (_nodeData)
//    {
//        auto d = std::dynamic_pointer_cast<NdsObjectData>(_nodeData);

//        m_editor->clear();

//        if (d->object())
//        {
//            GtObjectMemento mem = d->object()->toMemento();
//            m_editor->setPlainText(mem.toByteArray());
//        }
//    }
//    else
//    {
//        m_editor->clear();
//    }

//    Q_EMIT dataUpdated(0);
}
