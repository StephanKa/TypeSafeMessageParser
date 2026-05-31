#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <expected>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

namespace FieldRanges
{
    enum class ParseError : std::uint8_t
    {
        ValueNotExist,
        BelowRange,
        AboveRange,
        InvalidSize
    };

    /// Returns a human-readable string for a ParseError value.
    constexpr std::string_view to_string(ParseError error)
    {
        switch (error)
        {
        case ParseError::ValueNotExist: return "ValueNotExist";
        case ParseError::BelowRange: return "BelowRange";
        case ParseError::AboveRange: return "AboveRange";
        case ParseError::InvalidSize: return "InvalidSize";
        }
        return "Unknown";
    }

    // range definition for defining value with minimum and maximum values
    template<typename T>
    struct MinMaxRange
    {
        T min{};
        T max{};
    };

    // range definition where allowed entries can be specified
    template<typename... Entries>
    struct SpecificRange
    {
        constexpr explicit SpecificRange(Entries &&...entries) : possibleValues{ std::forward<Entries>(entries)... }
        {}

        using type = std::common_type_t<Entries...>;
        std::array<type, sizeof...(Entries)> possibleValues;
    };

    // concepts to determine the Range type
    template<typename Type>
    concept IsSpecificRange = requires(Type val) { val.possibleValues; };

    template<typename Type>
    concept IsMinMaxRange = requires(Type val) {
        val.min;
        val.max;
    };

    static_assert(!IsSpecificRange<MinMaxRange<int>>, "MinMaxRange must not satisfy IsSpecificRange");
    static_assert(!IsMinMaxRange<SpecificRange<int>>, "SpecificRange must not satisfy IsMinMaxRange");
}// namespace FieldRanges

/// Byte order specification for multi-byte field parsing.
enum class ByteOrder : std::uint8_t
{
    BigEndian,
    LittleEndian
};

// definition of a Field
template<std::size_t index,
    typename FieldType,
    auto Range = FieldRanges::MinMaxRange{ .min = std::numeric_limits<uint32_t>::min(), .max = std::numeric_limits<uint32_t>::max() },
    ByteOrder Order = ByteOrder::BigEndian>
struct FieldConfiguration
{
    static constexpr std::size_t byteIndex{ index };
    static constexpr std::size_t byteLength{ sizeof(FieldType) };
    static constexpr FieldType type{};
    static constexpr decltype(Range) range{ Range };
    static constexpr ByteOrder byteOrder{ Order };
};

// MessageParser implementation
namespace MessageParser
{
    template<typename Field>
    constexpr decltype(Field::type) getField(const auto &msg)
    {
        using Type = decltype(Field::type);
        return static_cast<Type>(msg.at(Field::byteIndex));
    }

    template<typename... Fields>
    consteval auto getSize()
    {
        std::size_t returnValue{ 0 };
        ((returnValue += Fields::byteLength), ...);
        return returnValue;
    }

    template<typename T, std::size_t size>
    consteval auto getMessageSize([[maybe_unused]] const std::array<T, size> &value)
    {
        return size;
    }

    template<typename T, std::size_t size, typename... Fields>
    constexpr void parseMessage(const std::array<T, size> &msg, [[maybe_unused]] const Fields &...fields)
    {
        if consteval
        {
            static_assert(getSize<Fields...>() == size, "size is not equal!");
        }
        ((getField<Fields>(msg)), ...);
    }

    template<typename Field>
    constexpr std::expected<decltype(Field::type), FieldRanges::ParseError> RangeChecker(const auto &value)
    {
        using RangeType = decltype(Field::range);
        if constexpr (FieldRanges::IsSpecificRange<RangeType>)
        {
            const auto res = std::ranges::find_if(Field::range.possibleValues, [&value](const auto &val) { return val == value; });
            if (res != Field::range.possibleValues.end())
            {
                return value;
            }
            return std::unexpected(FieldRanges::ParseError::ValueNotExist);
        }
        else if constexpr (FieldRanges::IsMinMaxRange<RangeType>)
        {
            if (value < Field::range.min)
            {
                return std::unexpected(FieldRanges::ParseError::BelowRange);
            }
            if (value > Field::range.max)
            {
                return std::unexpected(FieldRanges::ParseError::AboveRange);
            }
            return value;
        }
        else
        {
            return std::unexpected(FieldRanges::ParseError::ValueNotExist);
        }
    }

    constexpr auto getType([[maybe_unused]] const auto &field)
    {
        using Type = decltype(field.type);
        if constexpr (std::is_enum_v<decltype(field.type)>)
        {
            using UnderlyingType = std::underlying_type_t<Type>;
            return UnderlyingType{};
        }
        else
        {
            return Type{};
        }
    }

    /// Returns the underlying integral value of an enum, or the value itself for non-enums.
    /// Uses C++23 std::to_underlying for enum types.
    constexpr auto toUnderlying(const auto &value)
    {
        using Type = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_enum_v<Type>)
        {
            return std::to_underlying(value);
        }
        else
        {
            return value;
        }
    }

    template<typename Field>
    constexpr auto convertByteType(const auto &msg, [[maybe_unused]] const Field &field)
    {
        using Type = std::remove_cvref_t<decltype(Field::type)>;
        if constexpr (Field::byteLength == 1)
        {
            const auto tempValue = static_cast<Type>(msg.at(Field::byteIndex));
            return RangeChecker<Field>(tempValue);
        }
        else
        {
            auto tempValue = getType(field);
            using ReturnType = decltype(tempValue);

            if constexpr (Field::byteOrder == ByteOrder::BigEndian)
            {
                for (std::size_t i = 1; i <= Field::byteLength; ++i)
                {
                    constexpr std::size_t BITS_PER_BYTE{ 8 };
                    tempValue |= (static_cast<ReturnType>(msg.at(Field::byteIndex + Field::byteLength - i)) << ((i - 1) * BITS_PER_BYTE));
                }
            }
            else // LittleEndian
            {
                for (std::size_t i = 0; i < Field::byteLength; ++i)
                {
                    constexpr std::size_t BITS_PER_BYTE{ 8 };
                    tempValue |= (static_cast<ReturnType>(msg.at(Field::byteIndex + i)) << (i * BITS_PER_BYTE));
                }
            }
            return RangeChecker<Field>(static_cast<Type>(tempValue));
        }
    }

    /// Converts all fields from a message at compile time, returning a tuple of results.
    template<typename T, std::size_t size, typename... Fields>
    constexpr auto convertAll(const std::array<T, size> &msg, const Fields &...fields)
    {
        return std::make_tuple(convertByteType(msg, fields)...);
    }

    /// Checks whether all fields in the message have valid values.
    template<typename T, std::size_t size, typename... Fields>
    constexpr bool validateMessage(const std::array<T, size> &msg, const Fields &...fields)
    {
        return (convertByteType(msg, fields).has_value() && ...);
    }

    /// Concept that constrains a type to be a valid FieldConfiguration.
    template<typename T>
    concept IsFieldConfiguration = requires {
        { T::byteIndex } -> std::convertible_to<std::size_t>;
        { T::byteLength } -> std::convertible_to<std::size_t>;
        T::type;
        T::range;
    };
}// namespace MessageParser
