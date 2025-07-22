/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTGROUP_H
#define GT_INTELLI_COMMENTGROUP_H

#include <gt_objectgroup.h>
#include <intelli/globals.h>

namespace intelli
{

class CommentData;

/**
 * @brief The CommentGroup class.
 * Organizes all comments for a local graph.
 */
class CommentGroup : public GtObjectGroup
{
    Q_OBJECT

public:

    Q_INVOKABLE CommentGroup(GtObject* parent = nullptr);
    ~CommentGroup();
    
    QList<CommentData*> comments();
    QList<CommentData const*> comments() const;

    CommentData* findCommentByUuid(ObjectUuid const& uuid);
    CommentData const* findCommentByUuid(ObjectUuid const& uuid) const;

    CommentData* appendComment(std::unique_ptr<CommentData> comment);

signals:
    
    void commentAppended(CommentData*);
    
    void commentAboutToBeDeleted(CommentData*);

protected:

    void onObjectDataMerged() override;

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTGROUP_H
