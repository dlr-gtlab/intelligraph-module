/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 3.4.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#include "intelli/node.h"

#include "intelli/graphexecmodel.h"
#include "intelli/nodedatafactory.h"
#include "intelli/nodeexecutor.h"
#include "intelli/private/node_impl.h"
#include "intelli/private/utils.h"

#include <gt_qtutilities.h>
#include <gt_coreapplication.h>

#include <QRegExpValidator>
#include <QVBoxLayout>

using namespace intelli;
using namespace intelli::lib;

NodeDataPtr
intelli::convert(NodeDataPtr const& data, TypeId const& to)
{
    // forwarding call
    return NodeDataFactory::instance().convert(data, to);
}

namespace
{

template <typename N, typename P>
inline auto* dataInterface(N* node, P& pimpl)
{
    NodeDataInterface* model = pimpl->dataInterface;

    if (!model) model = NodeExecutor::accessExecModel(*const_cast<Node*>(node));

    return model;
}

} // namespace

std::unique_ptr<QWidget>
intelli::makeBaseWidget()
{
    auto base = std::make_unique<QWidget>();
    auto* layout = new QVBoxLayout(base.get());
    layout->setContentsMargins(0, 0, 0, 0);
    return base;
}

Node::Node(QString const& modelName, GtObject* parent) :
    GtObject(parent),
    lib::NodeBase(modelName),
    pimpl(std::make_unique<NodeImpl>())
{
    setFlag(UserDeletable, true);
    setFlag(UserRenamable, false);

    static const QString catData = QStringLiteral("Node-Data");
    registerProperty(pimpl->id, catData);
    registerProperty(pimpl->posX, catData);
    registerProperty(pimpl->posY, catData);
    registerProperty(pimpl->sizeWidth, catData);
    registerProperty(pimpl->sizeHeight, catData);

    static const QString catEval = QStringLiteral("Node-Evaluation");
    registerProperty(pimpl->isActive, catEval);

    pimpl->id.setReadOnly(true);
    pimpl->posX.setReadOnly(true);
    pimpl->posY.setReadOnly(true);
    pimpl->sizeWidth.setReadOnly(true);
    pimpl->sizeHeight.setReadOnly(true);

    bool hide = !gtApp || !gtApp->devMode();
    pimpl->posX.hide(hide);
    pimpl->posY.hide(hide);
    pimpl->sizeWidth.hide(hide);
    pimpl->sizeHeight.hide(hide);
    
    connect(this, &Node::portInserted,
            this, &Node::nodeChanged);
    connect(this, &Node::portDeleted,
            this, &Node::nodeChanged);
    connect(this, &QObject::objectNameChanged,
            this, &Node::nodeChanged);

    connect(&pimpl->id, &GtAbstractProperty::changed, this, [this](){
        impl.id = NodeId::fromValue(pimpl->id.get());
    });
    connect(&pimpl->posX, &GtAbstractProperty::changed, this, [this](){
        impl.pos.rx() = pimpl->posX.get();
    });
    connect(&pimpl->posY, &GtAbstractProperty::changed, this, [this](){
        impl.pos.ry() =  pimpl->posY.get();
    });
    connect(&pimpl->sizeHeight, &GtAbstractProperty::changed, this, [this](){
        impl.size.rheight() =  pimpl->sizeHeight.get();
    });
    connect(&pimpl->sizeWidth, &GtAbstractProperty::changed, this, [this](){
        impl.size.rwidth() =  pimpl->sizeWidth.get();
    });
    connect(&pimpl->isActive, &GtAbstractProperty::changed, this, [this](){
        impl.isActive = pimpl->isActive;
        if (impl.isActive) emit triggerNodeEvaluation();
    });
    connect(this, &QObject::objectNameChanged, this, [this](){
        impl.caption = objectName();
    });

    connect(this, &Node::computingStarted, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, true);
    }, Qt::DirectConnection);

    connect(this, &Node::computingFinished, this, [this](){
        setNodeFlag(NodeFlag::Evaluating, false);
        emit evaluated();
        }, Qt::DirectConnection);
}

void
Node::onNodeChange(NodeChange change)
{
    switch (change)
    {
    case NodeChange::Id:
        pimpl->id = id();
        break;
    case NodeChange::Position:
        pimpl->posX = pos().x();
        pimpl->posY = pos().y();
        break;
    case NodeChange::Size:
        pimpl->sizeHeight = size().height();
        pimpl->sizeWidth  = size().width();
        break;
    case NodeChange::Caption:
        updateObjectName();
        break;
    case NodeChange::IsActive:
        pimpl->isActive = isActive();
        emit isActiveChanged();
        break;
    }
}

void
Node::onPortsChange(PortsChange change, PortType type, PortIndex idx)
{
    switch (change)
    {
    case PortsChange::BeforeInsertion:
        emit portAboutToBeInserted(type, idx);
        break;
    case PortsChange::Inserted:
        emit portInserted(type, idx);
        break;
    case PortsChange::BeforeDeletion:
        emit portAboutToBeDeleted(type, idx);
        break;
    case PortsChange::Deleted:
        emit portDeleted(type, idx);
        break;
    }
}

Node::~Node()
{
    emit nodeAboutToBeDeleted(id());
}

void
Node::updateObjectName()
{
    gt::setUniqueName(*this, baseObjectName());
}


QString
Node::baseObjectName() const
{
    static QRegularExpression const regExp(QStringLiteral(R"((.+)(\s?\[\d+\]))"));
    constexpr int groupNo = 1;

    auto const& name = caption();
    auto const& match = regExp.match(name);
    return (match.capturedLength(groupNo) > 0) ? match.captured(groupNo) : name;
}

Node::NodeDataPtr
Node::nodeData(PortId id) const
{
    auto* model = dataInterface(this, pimpl);
    if (!model)
    {
        gtWarning().nospace()
            << objectName() << ": "
            << tr("Failed to access node data, evaluation model not found!");
        return {};
    }

    return model->nodeData(this->id(), id);
}

bool
Node::setNodeData(PortId id, NodeDataPtr data)
{
    auto* model = dataInterface(this, pimpl);
    if (!model)
    {
        gtWarning().nospace()
            << objectName() << ": "
            << tr("Failed to set node data, evaluation model not found!");
        return false;
    }

    return model->setNodeData(this->id(), id, std::move(data));
}

bool
Node::isPortConnected(PortId portId) const
{
    auto* graph = qobject_cast<Graph const*>(parent());
    if (!graph) return false;

    return !graph->findConnectedNodes(id(), portId).empty();
}

bool
Node::handleNodeEvaluation(GraphExecutionModel& model)
{
    switch (impl.evalMode)
    {
    case NodeEvalMode::Exclusive:
    case NodeEvalMode::Detached:
        return detachedEvaluation(*this, model);
    case NodeEvalMode::MainThread:
        return blockingEvaluation(*this, model);
    }

    gtError().nospace()
        << objectName() << ": "
        << tr("Unkonw eval mode! (%1)").arg((int)impl.evalMode);
    return false;
}

void
Node::registerWidgetFactory(WidgetFactory factory)
{
    pimpl->widgetFactory = std::move(factory);
}

void
Node::registerWidgetFactory(WidgetFactoryNoArgs factory)
{
    registerWidgetFactory([f = std::move(factory)](Node&){
        return f();
    });
}
