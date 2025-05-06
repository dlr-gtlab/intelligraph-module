/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGROUP_H
#define GT_INTELLI_COMMENTGROUP_H

#include <gt_objectgroup.h>

namespace intelli
{

class CommentObject;
class CommentGroup : public GtObjectGroup
{
    Q_OBJECT

public:

    Q_INVOKABLE CommentGroup(GtObject* parent = nullptr);
    ~CommentGroup();

    QList<CommentObject*> comments();
    QList<CommentObject const*> comments() const;

    CommentObject* appendComment(std::unique_ptr<CommentObject> comment);

signals:

    void commentAppended(CommentObject*);

    void commentAboutToBeDeleted(CommentObject*);

protected:

    void onObjectDataMerged() override;

private:

    /// List of all comments, used soely for identifying pruposes
    QVector<void*> m_comments;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGROUP_H
