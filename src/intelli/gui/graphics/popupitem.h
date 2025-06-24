/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_POPUPITEM_H
#define GT_INTELLI_POPUPITEM_H

#include <intelli/globals.h>

#include <QGraphicsItem>
#include <QTimeLine>

#include <chrono>

namespace intelli
{

/**
 * @brief The PopupItem class. For displaying a popup in a graphics scene. Only
 * one popup instance is visible at a time and each popup is dated.
 */
class PopupItem : public QGraphicsItem
{
public:

    using seconds = std::chrono::seconds;

    PopupItem(QGraphicsScene& scene, QString const& text, seconds timeout);
    ~PopupItem();

    static PopupItem* addPopupItem(QGraphicsScene& scene,
                                   QString const& text,
                                   seconds timeout)
    {
        return new PopupItem(scene, std::move(text), timeout);
    }

    static void clearActivePopups();

    /**
     * @brief Bounding rect of this object
     * @return Bounding rect
     */
    QRectF boundingRect() const override;

protected:

    void paint(QPainter* painter,
               QStyleOptionGraphicsItem const* option,
               QWidget* widget = nullptr) override;

private:

    /// Timeline for anmation
    QTimeLine m_timeLine;
};

} // namespace intelli

#endif // POPUPITEM_H
