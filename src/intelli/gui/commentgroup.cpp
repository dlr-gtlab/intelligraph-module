/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/gui/commentgroup.h>
#include <intelli/gui/commentobject.h>

#include <gt_qtutilities.h>

using namespace intelli;

CommentGroup::CommentGroup(GtObject* parent) :
    GtObjectGroup(parent)
{
    setObjectName(QStringLiteral("comments"));
}

QList<CommentObject*>
CommentGroup::comments()
{
    return findDirectChildren<CommentObject*>();
}

QList<CommentObject const*>
CommentGroup::comments() const
{
    return gt::container_const_cast(
        const_cast<CommentGroup*>(this)->comments()
    );
}

CommentGroup::~CommentGroup() = default;

CommentObject*
CommentGroup::appendComment(std::unique_ptr<CommentObject> comment)
{
    if (!appendChild(comment.get())) return {};

    connect(comment.get(), &CommentObject::aboutToBeDeleted,
            this, [this, c = comment.get()](){
        m_comments.removeOne(c);
        emit commentAboutToBeDeleted(c);
    });

    m_comments.append(comment.get());
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
        if (m_comments.contains(comment)) continue;

        std::unique_ptr<CommentObject> ptr{comment};
        ptr->setParent(nullptr);

        appendComment(std::move(ptr));
    }
}
