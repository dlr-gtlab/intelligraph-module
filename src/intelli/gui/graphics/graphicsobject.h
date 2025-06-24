/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_GRAPHICSOBJECT_H
#define GT_INTELLI_GRAPHICSOBJECT_H

#include <intelli/globals.h>
#include <intelli/exports.h>

#include <QGraphicsObject>

namespace intelli
{

// TODO: description
template <size_t Pattern>
static constexpr unsigned make_graphics_type()
{
    constexpr size_t total_bits = 31;
    constexpr size_t end_of_user_type = 16;
    constexpr size_t available_bits = total_bits - end_of_user_type;
    // check if pattern requires more bits than available
    static_assert(Pattern < (0b1 << available_bits),
                  "Exceeded available bits");
    // check if pattern requires more bits than available
    static_assert((1 << end_of_user_type) == QGraphicsItem::UserType,
                  "QGraphicsItem::UserType has invalid format!");
    // make sure bitwidth is sufficient
    static_assert(sizeof(QGraphicsItem::UserType) * 8 >= (total_bits + 1),
                  "QGraphicsItem::UserType has invalid format!");

    constexpr unsigned graphics_base_type = 1 << (available_bits - 1);

    return (graphics_base_type | Pattern) << (end_of_user_type + 1);
}

// TODO: description
template <GraphicsItemType ItemType, typename BaseClass>
static constexpr unsigned make_graphics_type()
{
    return (unsigned)make_graphics_type<(1 << (unsigned)ItemType)>() |
           (unsigned)BaseClass::Type;
}

// TODO: description
template <typename T,
          typename U,
          typename T_decay = std::remove_pointer_t<T>,
          typename U_decay = std::remove_pointer_t<U>>
T graphics_cast(U* u)
{
    constexpr unsigned mask = ~(unsigned)(QGraphicsItem::UserType - 1);
    constexpr unsigned base = make_graphics_type<0>();

    static_assert((T_decay::Type & mask & base) == base,
                  "T is not derived of intelli::GraphicsObject");

    if (!u) return nullptr;

    unsigned const type = u->type();
    if ((type & mask & T_decay::Type) != T_decay::Type) return nullptr;
    assert(dynamic_cast<T>(u));
    return static_cast<T>(u);
}

/// const overload
template <typename T,
          typename U,
          typename T_decay = std::remove_pointer_t<T>,
          typename U_decay = std::remove_pointer_t<U>,
          typename std::enable_if_t<!std::is_const<T_decay>::value, bool> = true>
T_decay const* graphics_cast(U const* u)
{
    return graphics_cast<T_decay const*>(u);
}

// TODO: description
class GT_INTELLI_EXPORT GraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:

    /// Flag hinting how an graphics object can be deleted. An object may
    /// not be able to be deleted in bulk because a confirmation dialog will
    /// pop up
    enum DeletableFlag
    {
        DefaultDeletable = 0,
        NotBulkDeletable, // object can be deleted if it is selected soely
        NotDeletable // object cannot be deleted by the user at all
    };

    /// Some objects may need to be deleted before others. Since deleting the
    /// associated object will for the most part delete the graphics object
    /// immediatly after, race conditions may arise. E.g. deleting a node
    /// causes all its connections to be deleted first. Thus connections
    /// should be deleted before deleting a node.
    enum DeleteOrdering
    {
        DefaultDeleteOrdering = 0,
        DeleteFirst = -1,
        DeleteLast = 1
    };

    enum { Type = make_graphics_type<0>() };
    int type() const override = 0;

    /**
     * @brief Returns whether this object is currently hovered.
     * @return Is hovered
     */
    bool isHovered() const { return m_hovered; }

    /**
     * @brief Returns a flag indicating how this object can be deleted, if at
     * all.
     * @return Deletable flag
     */
    virtual DeletableFlag deletableFlag() const { return DefaultDeletable; }

    /**
     * @brief Returns a flag indicating whether this object should be deleted
     * before objects with lower ordering.
     * @return Order flag
     */
    virtual DeleteOrdering deleteOrdering() const { return DefaultDeleteOrdering; }

    /**
     * @brief Implements how this object is deleted.
     * @return Success.
     */
    virtual bool deleteObject() = 0;

protected:

    GraphicsObject(QGraphicsItem* parent = nullptr);

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

signals:

    void hoveredChanged(QPrivateSignal);

private:

    bool m_hovered = false;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHICSOBJECT_H
