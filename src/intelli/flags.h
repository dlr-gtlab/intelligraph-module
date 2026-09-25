/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2025 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FLAGS_H
#define GT_INTELLI_FLAGS_H

#include <type_traits>
#include <stddef.h>

namespace intelli
{

//
template<typename Enum>
class UFlags
{
    using value_type = std::underlying_type_t<Enum>;
    value_type flags{};

    static_assert(!std::is_signed_v<std::underlying_type_t<Enum>>, "Expected unsigned type");

    constexpr static inline value_type
    recursive_initializer(typename std::initializer_list<Enum>::const_iterator it,
                          typename std::initializer_list<Enum>::const_iterator end) noexcept
    {
        return it == end ?
                   value_type{} :
                   (value_type(*it) | recursive_initializer(it + 1, end));
    }

public:

    constexpr UFlags() noexcept {}
    constexpr UFlags(Enum _flags) noexcept : flags(value_type(_flags)) {}
    constexpr UFlags(value_type _flags) noexcept : flags(_flags) {}
    constexpr UFlags(std::initializer_list<Enum> _flags) noexcept : flags(recursive_initializer(_flags)) {}

    constexpr inline UFlags& operator&=(UFlags mask) noexcept { flags &= mask.flags; return *this; }
    constexpr inline UFlags& operator&=(Enum mask) noexcept { flags &= value_type{mask}; return *this; }
    constexpr inline UFlags& operator|=(UFlags other) noexcept { flags |= other.flags; return *this; }
    constexpr inline UFlags& operator|=(Enum other) noexcept { flags |= value_type{other}; return *this; }
    constexpr inline UFlags& operator^=(UFlags other) noexcept { flags ^= other.flags; return *this; }
    constexpr inline UFlags& operator^=(Enum other) noexcept { flags ^= value_type{other}; return *this; }
    constexpr inline UFlags operator~() const noexcept { return {~flags}; }

    constexpr inline operator value_type() const noexcept { return flags; }

    constexpr inline bool testFlag(Enum flag) const noexcept
    {
        return (flags & value_type{flag}) == value_type{flag} &&
               (value_type{flag} != 0 || flags == value_type{flag} );
    }
    constexpr inline UFlags& setFlag(Enum flag, bool enable = true) noexcept
    {
        return enable ? (*this |= flag) : (*this &= ~value_type{flag});
    }
};

} // namespace intelli

template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator&(intelli::UFlags<Enum> flags, typename intelli::UFlags<Enum> mask) noexcept { return flags.operator&=(mask); }
template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator&(intelli::UFlags<Enum> flags, Enum mask) noexcept { return flags.operator&=(mask); }
template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator|(intelli::UFlags<Enum> flags, typename intelli::UFlags<Enum> other) noexcept { return flags.operator|=(other); }
template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator|(intelli::UFlags<Enum> flags, Enum other) noexcept { return flags.operator|=(other); }
template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator^(intelli::UFlags<Enum> flags, typename intelli::UFlags<Enum> other) noexcept { return flags.operator^=(other); }
template<typename Enum>
constexpr inline intelli::UFlags<Enum> operator^(intelli::UFlags<Enum> flags, Enum other) noexcept { return flags.operator^=(other); }

#endif // GT_INTELLI_FLAGS_H
