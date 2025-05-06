/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/commentobject.h>

#include <gt_stringproperty.h>
#include <gt_intproperty.h>
#include <gt_doubleproperty.h>
#include <gt_boolproperty.h>

#include <gt_coreapplication.h>

#include <QSize>

using namespace intelli;

struct CommentObject::Impl
{
    Impl() { }

    /// text of comment
    GtStringProperty text{
        "text", QObject::tr("text"), QObject::tr("Comment text")
    };
    /// x position of comment
    GtDoubleProperty posX{
        "posX", QObject::tr("x-Pos"), QObject::tr("x-Position")
    };
    /// y position of comment
    GtDoubleProperty posY{
        "posY", QObject::tr("y-Pos"), QObject::tr("y-Position")
    };

    /// width of comment widget
    GtIntProperty sizeWidth{
        "sizeWidth",
        QObject::tr("Size width"),
        QObject::tr("Size width"), -1
    };
    /// height of comment widget
    GtIntProperty sizeHeight{
        "sizeHeight",
        QObject::tr("Size height"),
        QObject::tr("Size height"), -1
    };

    /// whether the comment is collapsed
    GtBoolProperty collapsed{
        "collapsed", QObject::tr("collapsed"), QObject::tr("collapsed"), false
    };


};

CommentObject::CommentObject(GtObject* parent) :
    GtObject(parent),
    pimpl(std::make_unique<Impl>())
{
    setFlag(UserDeletable);

    registerProperty(pimpl->posX);
    registerProperty(pimpl->posY);
    registerProperty(pimpl->sizeWidth);
    registerProperty(pimpl->sizeHeight);
    registerProperty(pimpl->collapsed);
    registerProperty(pimpl->text);

    pimpl->posX.setReadOnly(true);
    pimpl->posY.setReadOnly(true);
    pimpl->sizeWidth.setReadOnly(true);
    pimpl->sizeHeight.setReadOnly(true);
    pimpl->collapsed.setReadOnly(true);
    pimpl->text.setReadOnly(true);

#ifndef GT_INTELLI_DEBUG_NODE_PROPERTIES
    bool hide = !gtApp || !gtApp->devMode();
    pimpl->posX.hide(true);
    pimpl->posY.hide(true);
    pimpl->sizeWidth.hide(true);
    pimpl->sizeHeight.hide(true);
    pimpl->collapsed.hide(true);
    pimpl->text.hide(true);
#endif
    connect(&pimpl->collapsed, &GtAbstractProperty::changed, this, [this](){
        emit commentCollapsedChanged(pimpl->collapsed.get());
    });

    // position is changed in pairs -> sufficient to subscribe to changes
    // to y-pos (avoids emitting singal twice)
    connect(&pimpl->posY, &GtAbstractProperty::changed, this, [this](){
        emit commentPositionChanged();
    });

    setObjectName(QStringLiteral("comment_%1")
                      .arg(uuid().remove("{").remove("-").mid(0, 8)));
}

CommentObject::~CommentObject()
{
    emit aboutToBeDeleted();
}

void
CommentObject::setText(QString text)
{
    pimpl->text = std::move(text);
}

QString const&
CommentObject::text() const
{
    return pimpl->text.get();
}

void
CommentObject::setPos(Position pos)
{
    if (this->pos() != pos)
    {
        pimpl->posX = pos.x();
        pimpl->posY = pos.y();
        changed();
    }
}

Position
CommentObject::pos() const
{
    return { pimpl->posX, pimpl->posY };
}

void
CommentObject::setSize(QSize size)
{
    if (this->size() != size)
    {
        pimpl->sizeWidth = size.width();
        pimpl->sizeHeight = size.height();
        changed();
    }
}

QSize
CommentObject::size() const
{
    return { pimpl->sizeWidth, pimpl->sizeHeight };
}

void
CommentObject::setCollapsed(bool collapsed)
{
    if (this->isCollapsed() != collapsed)
    {
        pimpl->collapsed = collapsed;
        changed();
    }
}

bool
CommentObject::isCollapsed() const
{
    return pimpl->collapsed;
}
