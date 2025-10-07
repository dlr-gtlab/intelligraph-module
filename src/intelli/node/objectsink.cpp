/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Jens Schmeink <jens.schmeink@dlr.de>
 */
#include "objectsink.h"

#include <intelli/data/object.h>
#include <intelli/nodedatainterface.h>

#include <QPushButton>

#include "gt_application.h"
#include "gt_datamodel.h"
#include "gt_project.h"

using namespace intelli;

ObjectSink::ObjectSink() :
    Node("Object sink"),
    m_in(addInPort({typeId<ObjectData>(), tr("Object")})),
    m_target("target", tr("Target"), tr("Target"), QString{},
             this, QStringList{ GT_CLASSNAME(GtObject) }, true)
{
    registerProperty(m_target);

    setNodeEvalMode(NodeEvalMode::Blocking);

    // registering the widget factory
    registerWidgetFactory([=](){
        auto w = std::make_unique<QPushButton>("Export");

        w->setEnabled(false);

        connect(this, &Node::inputDataRecieved,
                this, [this, w_ = w.get()](PortId portId){
            auto data_tmp = nodeData<ObjectData>(portId);
            w_->setEnabled(data_tmp != nullptr);
        });

        connect(w.get(), &QPushButton::clicked,
                this, [this](){ doExport(); });

        return w;
    });
}

void
ObjectSink::doExport()
{
    auto data = nodeData<ObjectData>(m_in);

    if (!data) return;

    const GtObject* source = data->object();

    QString targetUUID = m_target.getVal();

    if (targetUUID.isEmpty()) return;

    auto model = exec::nodeDataInterface(*this);

    if (!model || !model->scope()) return;

    GtObject* target = m_target.linkedObject(model->scope());

    if (!target) return;

    if (target->metaObject()->className() != source->metaObject()->className())
    {
        gtInfo() << tr("For source and target of different types the source is"
                       "appended to the target.");
        GtObject* sourceClone = source->clone();
        sourceClone->moveToThread(target->thread());

        gtDataModel->appendChild(sourceClone, target);
        return;
    }
    else
    {
        GtObject* sourceClone = source->clone();
        GtObject* targetParent = target->parentObject();
        QString oldUUID = target->uuid();
        QString oldName = target->objectName();
        sourceClone->moveToThread(targetParent->thread());

        GtCommand command = gtApp->startCommand(gtApp->currentProject(),
                                                tr("Overwrite target"));
        sourceClone->setUuid(oldUUID);
        sourceClone->setObjectName(oldName);
        delete target;
        targetParent->appendChild(sourceClone);

        gtApp->endCommand(command);

        return;
    }
}
