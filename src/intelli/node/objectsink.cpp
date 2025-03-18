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

#include <QPushButton>

#include "gt_application.h"
#include "gt_datamodel.h"
#include "gt_project.h"

using namespace intelli;

ObjectSink::ObjectSink() :
    Node("Object sink"),
    m_target("target", tr("Target"), tr("Target"), QString(), this,
               QStringList() << GT_CLASSNAME(GtObject), true)
{
    m_in  = addInPort({typeId<ObjectData>(), tr("Object")});

    registerProperty(m_target);

    // registering the widget factory
    registerWidgetFactory([=](intelli::Node& /*this*/){
        auto w = std::make_unique<QPushButton>("Export");

        connect(w.get(), &QPushButton::clicked, this,
                [=, w_ = w.get()](){
                    doExport();
                });

        connect(this, &ObjectSink::exportActivated, w.get(),
                [=, w_ = w.get()](bool a) {
                    w_->setEnabled(a);

                    if (a)
                    {
                        setButtonColor(w_, QColor(252, 186, 3));
                    }
                    else
                    {
                        w_->setStyleSheet({});
                    }
                });

        connect(this, &ObjectSink::exportFinished, w.get(),
                [=, w_ = w.get()](bool a) {
                    if (a)
                    {
                        setButtonColor(w_, QColor(0, 100, 10));
                    }
                    else
                    {
                        setButtonColor(w_, QColor(150, 40, 0));
                    }
                });

        return w;
    });
}

void
intelli::ObjectSink::eval()
{
    auto data_tmp = nodeData<ObjectData>(m_in);

    if (data_tmp)
    {
        emit exportActivated(true);
    }
    else
    {
        emit exportActivated(false);
    }
}

void
ObjectSink::setButtonColor(QPushButton* button, const QColor& col)
{
    if (!button) return;

    static const QString styleBase = QStringLiteral(R"(
      QAbstractButton{
        margin: 2px;
        max-width: 40px; min-height: 20px;
        border: 1px solid #777777; border-radius: 2px;
        background: qlineargradient( x1:0 y1:0, x2:0 y2:1, stop:0 %3, stop:1 '%1');
      }
      QAbstractButton:hover{ background-color: %2}
      QAbstractButton:pressed{ background-color: %2;})");


    QColor highlightColor = col.lighter();
    QColor gradientUpColor = col.lighter(110);
    QColor gradientLoColor = col.darker(110);

    QString style = QString(styleBase).arg(QVariant(gradientLoColor).toString(),

                                           QVariant(highlightColor).toString(),
                                           QVariant(gradientUpColor).toString());

    button->setStyleSheet(style);
    button->update();
}

void
ObjectSink::doExport()
{
    auto data = nodeData<ObjectData>(m_in);

    const GtObject* source = data->object();

    QString targetUUID = m_target.getVal();

    if (targetUUID.isEmpty()) return;

    GtProject* currProj = gtApp->currentProject();

    if (!currProj) return;

    GtObject* target = m_target.linkedObject(currProj);

    if (!target) return;

    GtObject* sourceClone = source->clone();
    sourceClone->moveToThread(target->thread());

    gtDataModel->appendChild(sourceClone, target);
}
