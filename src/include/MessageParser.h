#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <spdlog/spdlog.h>
#include <type_traits>

namespace FieldRanges
{
    enum class ParseError
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
} // namespace FieldRanges

// definition of a Field
template<std::size_t index, typename FieldType, auto Range = FieldRanges::MinMaxRange{ .min = std::numeric_limits<uint32_t>::min(), .max = std::numeric_limits<uint32_t>::max() }>
struct FieldConfiguration
{
    static constexpr std::size_t byteIndex{ index };
    static constexpr std::size_t byteLength{ sizeof(FieldType) };
    static constexpr FieldType type{};
    static constexpr std::size_t startBit{ 0 };
    static constexpr std::size_t bitLength{ 8 };
    static constexpr bool isBitAligned{ bitLength % 8 == 0 };
    static constexpr decltype(Range) range{ Range };
};

// MessageParser implementation
namespace MessageParser
{
    template<typename Field>
    constexpr decltype(Field::type) getField(const auto &msg)
    {
        using Type = decltype(Field::type);
        const auto casted = static_cast<Type>(msg.at(Field::byteIndex));
        spdlog::info("value {}", static_cast<uint32_t>(casted));
        return casted;
    }

    template<typename... Fields>
    constexpr auto getSize()
    {
        std::size_t returnValue{0};
        ((returnValue += Fields::byteLength), ...);
        return returnValue;
    }

    template<typename T, size_t size>
    constexpr auto getMessageSize([[maybe_unused]]const std::array<T, size>& value)
    {
        return size;
    }

    template<typename T, size_t size, typename... Fields>
    constexpr void parseMessage(const std::array<T, size>& msg, [[maybe_unused]] const Fields &...fields)
    {
        if consteval
        {
            static_assert(getSize<Fields...>() == size, "size is not equal!");
        }
        ((getField<Fields>(msg)), ...);
    }

    template<typename Field>
    constexpr std::optional<decltype(Field::type)> RangeChecker(const auto &value)
    {
        using RangeType = decltype(Field::range);
        if constexpr (FieldRanges::IsSpecificRange<RangeType>)
        {
            const auto res = std::find_if(Field::range.possibleValues.begin(), Field::range.possibleValues.end(), [&value](const auto &val) { return val == value; });
            if (res != Field::range.possibleValues.end())
            {
                return value;
            }
        }
        if constexpr (FieldRanges::IsMinMaxRange<RangeType>)
        {
            if (value >= Field::range.min && value <= Field::range.max)
            {
                return value;
            }
        }
        return std::nullopt;
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
    constexpr std::optional<decltype(Field::type)> convertByteType(const auto &msg, [[maybe_unused]] const Field &field)
    {
        using Type = std::remove_cvref_t<decltype(Field::type)>;
        if constexpr (Field::byteLength == 1)
        {
            const auto tempValue = static_cast<Type>(msg.at(Field::byteIndex));
            return RangeChecker<Field>(tempValue);
        }
        else
        {
            constexpr size_t BITS_PER_BYTE{ 8 };
            auto tempValue = getType(field);
            using ReturnType = decltype(tempValue);
            for (size_t i = 1; i <= Field::byteLength; ++i)
            {
                tempValue |= static_cast<ReturnType>(msg.at(Field::byteIndex + Field::byteLength - i) << ((i - 1) * BITS_PER_BYTE));
            }
            return RangeChecker<Field>(static_cast<Type>(tempValue));
        }
    }
}// namespace MessageParser
