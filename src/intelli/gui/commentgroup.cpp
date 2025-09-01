/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/commentgroup.h>
#include <intelli/gui/commentdata.h>

#include <gt_qtutilities.h>

using namespace intelli;

struct CommentGroup::Impl
{
    /// List of all comments, used soely for identifying pruposes
    /// for onObjectDataMerged
    QVector<void*> comments;
};

CommentGroup::CommentGroup(GtObject* parent) :
    GtObjectGroup(parent),
    pimpl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("comments"));
}

CommentGroup::~CommentGroup() = default;

QList<CommentData*>
CommentGroup::comments()
{
    return findDirectChildren<CommentData*>();
}

QList<CommentData const*>
CommentGroup::comments() const
{
    return gt::container_const_cast(
        const_cast<CommentGroup*>(this)->comments()
    );
}

CommentData*
CommentGroup::findCommentByUuid(ObjectUuid const& uuid)
{
    return qobject_cast<CommentData*>(getObjectByUuid(uuid));
}

CommentData const*
CommentGroup::findCommentByUuid(ObjectUuid const& uuid) const
{
    return qobject_cast<CommentData const*>(getObjectByUuid(uuid));
}

CommentData*
CommentGroup::appendComment(std::unique_ptr<CommentData> comment)
{
    if (!appendChild(comment.get())) return {};
    
    connect(comment.get(), &CommentData::aboutToBeDeleted,
            this, [this, c = comment.get()](){
        pimpl->comments.removeOne(c);
        emit commentAboutToBeDeleted(c);
    });

    pimpl->comments.append(comment.get());
    emit commentAppended(comment.get());

    return comment.release();
}

void
CommentGroup::onObjectDataMerged()
{
    GtObject::onObjectDataMerged();

    auto const& comments = this->comments();
    for (auto* comment : comments)
    {
        if (pimpl->comments.contains(comment)) continue;
        
        std::unique_ptr<CommentData> ptr{comment};
        ptr->setParent(nullptr);

        appendComment(std::move(ptr));
    }
}
