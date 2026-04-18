#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <limits>
#include <type_traits>

namespace FieldRanges
{
    enum class ParseError : std::uint8_t
    {
        ValueNotExist,
        BelowRange,
        AboveRange
    };

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

// definition of a Field
template<std::size_t index, typename FieldType, auto Range = FieldRanges::MinMaxRange{ .min = std::numeric_limits<uint32_t>::min(), .max = std::numeric_limits<uint32_t>::max() }>
struct FieldConfiguration
{
    static constexpr std::size_t byteIndex{ index };
    static constexpr std::size_t byteLength{ sizeof(FieldType) };
    static constexpr FieldType type{};
    static constexpr decltype(Range) range{ Range };
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
    constexpr auto getSize()
    {
        size_t returnValue{ 0 };
        ((returnValue += Fields::byteLength), ...);
        return returnValue;
    }

    template<typename T, size_t size>
    constexpr auto getMessageSize([[maybe_unused]] const std::array<T, size> &value)
    {
        return size;
    }

    template<typename T, size_t size, typename... Fields>
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
            for (size_t i = 1; i <= Field::byteLength; ++i)
            {
                constexpr size_t BITS_PER_BYTE{ 8 };
                tempValue |= (static_cast<ReturnType>(msg.at(Field::byteIndex + Field::byteLength - i)) << ((i - 1) * BITS_PER_BYTE));
            }
            return RangeChecker<Field>(static_cast<Type>(tempValue));
        }
    }
}// namespace MessageParser
