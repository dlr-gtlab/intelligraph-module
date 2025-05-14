/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_COMMENTOBJECT_H
#define GT_INTELLI_COMMENTOBJECT_H

#include <intelli/globals.h>

#include <gt_object.h>

namespace intelli
{

/// TODO
class CommentObject : public GtObject
{
    Q_OBJECT

public:

    Q_INVOKABLE CommentObject(GtObject* parent = nullptr);
    ~CommentObject();

    void setText(QString text);

    QString const& text() const;

    void setPos(Position pos);

    Position pos() const;

    void setSize(QSize size);

    QSize size() const;

    void setCollapsed(bool collapse);

    bool isCollapsed() const;

    void appendConnection(ObjectUuid const& targetUuid);

    bool removeConnection(ObjectUuid const& targetUuid);

    bool isObjectConnected(ObjectUuid const& targetUuid);

    size_t nconnections() const;

    ObjectUuid const& connectionAt(size_t idx) const;

signals:

    void aboutToBeDeleted();

    void commentCollapsedChanged(bool collapsed);

    void commentPositionChanged();

    void connectionAppended(ObjectUuid const& objectUuid);

    void connectionRemoved(ObjectUuid const& objectUuid);

private:

    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace intelli

#endif // GT_INTELLI_COMMENTOBJECT_H
