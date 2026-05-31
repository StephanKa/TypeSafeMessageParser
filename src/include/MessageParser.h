#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <ostream>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace FieldRanges
{
    enum class ParseError : std::uint8_t
    {
        ValueNotExist,
        BelowRange,
        AboveRange,
        InvalidSize,
        ChecksumMismatch,
        CustomValidationFailed
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
        case ParseError::ChecksumMismatch: return "ChecksumMismatch";
        case ParseError::CustomValidationFailed: return "CustomValidationFailed";
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

    /// Custom validator predicate range - accepts a constexpr-callable predicate
    template<typename T, typename Pred>
    struct CustomRange
    {
        Pred predicate;
    };

    // concepts to determine the Range type
    template<typename Type>
    concept IsSpecificRange = requires(Type val) { val.possibleValues; };

    template<typename Type>
    concept IsMinMaxRange = requires(Type val) {
        val.min;
        val.max;
    };

    template<typename Type>
    concept IsCustomRange = requires(Type val) { val.predicate; };

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

// ============================================================================
// Bit-field support
// ============================================================================

/// Configuration for a field that spans partial bytes (bit-level access)
template<std::size_t byteIdx, std::size_t bitOffset, std::size_t bitWidth, typename FieldType,
    auto Range = FieldRanges::MinMaxRange{ .min = std::numeric_limits<uint32_t>::min(), .max = std::numeric_limits<uint32_t>::max() }>
struct BitFieldConfiguration
{
    static constexpr std::size_t byteIndex{ byteIdx };
    static constexpr std::size_t bitOff{ bitOffset };
    static constexpr std::size_t bitLen{ bitWidth };
    static constexpr std::size_t byteLength{ (bitOffset + bitWidth + 7) / 8 };
    static constexpr FieldType type{};
    static constexpr decltype(Range) range{ Range };
    static constexpr ByteOrder byteOrder{ ByteOrder::BigEndian };
};

// ============================================================================
// Named field support
// ============================================================================

/// A FieldConfiguration with an attached compile-time name for debug/logging
template<std::size_t index, typename FieldType,
    auto Range = FieldRanges::MinMaxRange{ .min = std::numeric_limits<uint32_t>::min(), .max = std::numeric_limits<uint32_t>::max() },
    ByteOrder Order = ByteOrder::BigEndian>
struct NamedFieldConfiguration
{
    static constexpr std::size_t byteIndex{ index };
    static constexpr std::size_t byteLength{ sizeof(FieldType) };
    static constexpr FieldType type{};
    static constexpr decltype(Range) range{ Range };
    static constexpr ByteOrder byteOrder{ Order };
    std::string_view name{};
};

// MessageParser implementation
namespace MessageParser
{
    // ========================================================================
    // Concepts
    // ========================================================================

    /// Concept that constrains a type to be a valid FieldConfiguration.
    template<typename T>
    concept IsFieldConfiguration = requires {
        { T::byteIndex } -> std::convertible_to<std::size_t>;
        { T::byteLength } -> std::convertible_to<std::size_t>;
        T::type;
        T::range;
    };

    /// Concept for bit-field configurations
    template<typename T>
    concept IsBitFieldConfiguration = requires {
        { T::byteIndex } -> std::convertible_to<std::size_t>;
        { T::bitOff } -> std::convertible_to<std::size_t>;
        { T::bitLen } -> std::convertible_to<std::size_t>;
        T::type;
        T::range;
    };

    /// Concept for named field configurations
    template<typename T>
    concept IsNamedFieldConfiguration = IsFieldConfiguration<T> && requires(T t) {
        { t.name } -> std::convertible_to<std::string_view>;
    };

    // ========================================================================
    // Core parsing functions
    // ========================================================================

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
        else if constexpr (FieldRanges::IsCustomRange<RangeType>)
        {
            if (Field::range.predicate(value))
            {
                return value;
            }
            return std::unexpected(FieldRanges::ParseError::CustomValidationFailed);
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

    /// Overload for std::span-based messages
    template<typename Field>
    constexpr auto convertByteType(std::span<const std::uint8_t> msg, [[maybe_unused]] const Field &field)
    {
        using Type = std::remove_cvref_t<decltype(Field::type)>;
        if constexpr (Field::byteLength == 1)
        {
            const auto tempValue = static_cast<Type>(msg[Field::byteIndex]);
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
                    tempValue |= (static_cast<ReturnType>(msg[Field::byteIndex + Field::byteLength - i]) << ((i - 1) * BITS_PER_BYTE));
                }
            }
            else
            {
                for (std::size_t i = 0; i < Field::byteLength; ++i)
                {
                    constexpr std::size_t BITS_PER_BYTE{ 8 };
                    tempValue |= (static_cast<ReturnType>(msg[Field::byteIndex + i]) << (i * BITS_PER_BYTE));
                }
            }
            return RangeChecker<Field>(static_cast<Type>(tempValue));
        }
    }

    // ========================================================================
    // Bit-field parsing
    // ========================================================================

    /// Parse a bit-field from a message byte array
    template<typename BitField>
        requires IsBitFieldConfiguration<BitField>
    constexpr auto convertBitField(const auto &msg, [[maybe_unused]] const BitField &field)
    {
        using Type = std::remove_cvref_t<decltype(BitField::type)>;

        // Assemble the raw bits from the relevant bytes
        std::uint32_t raw = 0;
        constexpr std::size_t totalBits = BitField::bitOff + BitField::bitLen;
        constexpr std::size_t bytesNeeded = (totalBits + 7) / 8;

        for (std::size_t i = 0; i < bytesNeeded; ++i)
        {
            raw |= (static_cast<std::uint32_t>(msg.at(BitField::byteIndex + i)) << ((bytesNeeded - 1 - i) * 8));
        }

        // Shift right to align the field to bit 0, then mask
        constexpr std::size_t shift = (bytesNeeded * 8) - totalBits;
        constexpr std::uint32_t mask = (BitField::bitLen >= 32) ? 0xFFFFFFFF : ((1u << BitField::bitLen) - 1u);
        const auto value = static_cast<Type>((raw >> shift) & mask);

        return RangeChecker<BitField>(value);
    }

    // ========================================================================
    // Floating-point field support
    // ========================================================================

    /// Parse a floating-point field (float or double) from raw bytes
    template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian>
    constexpr auto getFloatField(const auto &msg)
    {
        std::uint32_t raw = 0;
        if constexpr (Order == ByteOrder::BigEndian)
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                raw |= (static_cast<std::uint32_t>(msg.at(index + i)) << ((3 - i) * 8));
            }
        }
        else
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                raw |= (static_cast<std::uint32_t>(msg.at(index + i)) << (i * 8));
            }
        }
        return std::bit_cast<float>(raw);
    }

    template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian>
    constexpr auto getDoubleField(const auto &msg)
    {
        std::uint64_t raw = 0;
        if constexpr (Order == ByteOrder::BigEndian)
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                raw |= (static_cast<std::uint64_t>(msg.at(index + i)) << ((7 - i) * 8));
            }
        }
        else
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                raw |= (static_cast<std::uint64_t>(msg.at(index + i)) << (i * 8));
            }
        }
        return std::bit_cast<double>(raw);
    }

    // ========================================================================
    // Signed integer support
    // ========================================================================

    /// Parse a signed integer field - works with int8_t, int16_t, int32_t
    /// Uses the same FieldConfiguration but with signed types
    // (Already supported via FieldConfiguration with signed FieldType;
    //  the bit_cast and range checking work for signed types naturally)

    // ========================================================================
    // Batch operations
    // ========================================================================

    /// Converts all fields from a message at compile time, returning a tuple of results.
    template<typename T, std::size_t size, typename... Fields>
    constexpr auto convertAll(const std::array<T, size> &msg, const Fields &...fields)
    {
        return std::make_tuple(convertByteType(msg, fields)...);
    }

    /// Overload for std::span
    template<typename... Fields>
    constexpr auto convertAll(std::span<const std::uint8_t> msg, const Fields &...fields)
    {
        return std::make_tuple(convertByteType(msg, fields)...);
    }

    /// Checks whether all fields in the message have valid values.
    template<typename T, std::size_t size, typename... Fields>
    constexpr bool validateMessage(const std::array<T, size> &msg, const Fields &...fields)
    {
        return (convertByteType(msg, fields).has_value() && ...);
    }

    /// Overload for std::span
    template<typename... Fields>
    constexpr bool validateMessage(std::span<const std::uint8_t> msg, const Fields &...fields)
    {
        return (convertByteType(msg, fields).has_value() && ...);
    }

    // ========================================================================
    // Message serialization (encode)
    // ========================================================================

    /// Encode a single field value into a byte array
    template<typename Field, typename T, std::size_t size>
    constexpr void encodeField(std::array<T, size> &msg, [[maybe_unused]] const Field &field, const decltype(Field::type) &value)
    {
        using ValueType = std::remove_cvref_t<decltype(Field::type)>;
        using RawType = std::conditional_t<std::is_enum_v<ValueType>, std::underlying_type_t<ValueType>, ValueType>;

        RawType rawValue{};
        if constexpr (std::is_enum_v<ValueType>)
        {
            rawValue = std::to_underlying(value);
        }
        else
        {
            rawValue = value;
        }

        if constexpr (Field::byteLength == 1)
        {
            msg.at(Field::byteIndex) = static_cast<std::uint8_t>(rawValue);
        }
        else
        {
            constexpr std::size_t BITS_PER_BYTE{ 8 };
            if constexpr (Field::byteOrder == ByteOrder::BigEndian)
            {
                for (std::size_t i = 0; i < Field::byteLength; ++i)
                {
                    msg.at(Field::byteIndex + i) = static_cast<std::uint8_t>(
                        rawValue >> ((Field::byteLength - 1 - i) * BITS_PER_BYTE));
                }
            }
            else // LittleEndian
            {
                for (std::size_t i = 0; i < Field::byteLength; ++i)
                {
                    msg.at(Field::byteIndex + i) = static_cast<std::uint8_t>(rawValue >> (i * BITS_PER_BYTE));
                }
            }
        }
    }

    /// Encode a single field with range validation, returns expected
    template<typename Field, typename T, std::size_t size>
    constexpr std::expected<void, FieldRanges::ParseError> encodeFieldChecked(
        std::array<T, size> &msg, [[maybe_unused]] const Field &field, const decltype(Field::type) &value)
    {
        auto validated = RangeChecker<Field>(value);
        if (!validated.has_value())
        {
            return std::unexpected(validated.error());
        }
        encodeField(msg, field, value);
        return {};
    }

    /// Encode a float value into message bytes
    template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian, typename T, std::size_t size>
    constexpr void encodeFloatField(std::array<T, size> &msg, float value)
    {
        auto raw = std::bit_cast<std::uint32_t>(value);
        if constexpr (Order == ByteOrder::BigEndian)
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                msg.at(index + i) = static_cast<std::uint8_t>(raw >> ((3 - i) * 8));
            }
        }
        else
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                msg.at(index + i) = static_cast<std::uint8_t>(raw >> (i * 8));
            }
        }
    }

    /// Encode a double value into message bytes
    template<std::size_t index, ByteOrder Order = ByteOrder::BigEndian, typename T, std::size_t size>
    constexpr void encodeDoubleField(std::array<T, size> &msg, double value)
    {
        auto raw = std::bit_cast<std::uint64_t>(value);
        if constexpr (Order == ByteOrder::BigEndian)
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                msg.at(index + i) = static_cast<std::uint8_t>(raw >> ((7 - i) * 8));
            }
        }
        else
        {
            for (std::size_t i = 0; i < 8; ++i)
            {
                msg.at(index + i) = static_cast<std::uint8_t>(raw >> (i * 8));
            }
        }
    }

    // ========================================================================
    // Cross-field validation
    // ========================================================================

    /// Validates a message using a cross-field predicate that receives the full message
    template<typename T, std::size_t size, typename Predicate>
    constexpr bool validateCrossField(const std::array<T, size> &msg, Predicate &&pred)
    {
        return pred(msg);
    }

    /// Validates a message using multiple cross-field predicates (all must pass)
    template<typename T, std::size_t size, typename... Predicates>
    constexpr bool validateCrossFields(const std::array<T, size> &msg, Predicates &&...preds)
    {
        return (preds(msg) && ...);
    }

    // ========================================================================
    // Optional/conditional fields
    // ========================================================================

    /// Parse a field only if a condition is true; returns std::nullopt if condition is false
    template<typename Field>
    constexpr auto convertByteTypeIf(const auto &msg, [[maybe_unused]] const Field &field, bool condition)
        -> std::expected<std::optional<decltype(Field::type)>, FieldRanges::ParseError>
    {
        if (!condition)
        {
            return std::optional<decltype(Field::type)>{ std::nullopt };
        }
        auto result = convertByteType(msg, field);
        if (result.has_value())
        {
            return std::optional<decltype(Field::type)>{ *result };
        }
        return std::unexpected(result.error());
    }

    // ========================================================================
    // CRC / Checksum verification
    // ========================================================================

    /// CRC-8 (polynomial 0x07, init 0x00)
    constexpr std::uint8_t computeCrc8(std::span<const std::uint8_t> data)
    {
        std::uint8_t crc = 0x00;
        for (auto byte : data)
        {
            crc ^= byte;
            for (int i = 0; i < 8; ++i)
            {
                if (crc & 0x80)
                    crc = static_cast<std::uint8_t>((crc << 1) ^ 0x07);
                else
                    crc = static_cast<std::uint8_t>(crc << 1);
            }
        }
        return crc;
    }

    /// CRC-16 (CCITT, polynomial 0x1021, init 0xFFFF)
    constexpr std::uint16_t computeCrc16(std::span<const std::uint8_t> data)
    {
        std::uint16_t crc = 0xFFFF;
        for (auto byte : data)
        {
            crc ^= static_cast<std::uint16_t>(byte) << 8;
            for (int i = 0; i < 8; ++i)
            {
                if (crc & 0x8000)
                    crc = static_cast<std::uint16_t>((crc << 1) ^ 0x1021);
                else
                    crc = static_cast<std::uint16_t>(crc << 1);
            }
        }
        return crc;
    }

    /// CRC-32 (polynomial 0xEDB88320, reflected)
    constexpr std::uint32_t computeCrc32(std::span<const std::uint8_t> data)
    {
        std::uint32_t crc = 0xFFFFFFFF;
        for (auto byte : data)
        {
            crc ^= byte;
            for (int i = 0; i < 8; ++i)
            {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
            }
        }
        return ~crc;
    }

    /// Verify CRC-8 at a given position in the message
    template<std::size_t payloadStart, std::size_t payloadLen, std::size_t crcIndex, typename T, std::size_t size>
    constexpr std::expected<void, FieldRanges::ParseError> verifyCrc8(const std::array<T, size> &msg)
    {
        auto payload = std::span<const std::uint8_t>(msg.data() + payloadStart, payloadLen);
        auto expected_crc = msg.at(crcIndex);
        if (computeCrc8(payload) == expected_crc)
            return {};
        return std::unexpected(FieldRanges::ParseError::ChecksumMismatch);
    }

    /// Verify CRC-16 at a given position (big-endian CRC bytes)
    template<std::size_t payloadStart, std::size_t payloadLen, std::size_t crcIndex,
        ByteOrder CrcOrder = ByteOrder::BigEndian, typename T, std::size_t size>
    constexpr std::expected<void, FieldRanges::ParseError> verifyCrc16(const std::array<T, size> &msg)
    {
        auto payload = std::span<const std::uint8_t>(msg.data() + payloadStart, payloadLen);
        std::uint16_t stored_crc{};
        if constexpr (CrcOrder == ByteOrder::BigEndian)
        {
            stored_crc = static_cast<std::uint16_t>((msg.at(crcIndex) << 8) | msg.at(crcIndex + 1));
        }
        else
        {
            stored_crc = static_cast<std::uint16_t>(msg.at(crcIndex) | (msg.at(crcIndex + 1) << 8));
        }
        if (computeCrc16(payload) == stored_crc)
            return {};
        return std::unexpected(FieldRanges::ParseError::ChecksumMismatch);
    }

    /// Simple 8-bit checksum (sum of bytes & 0xFF)
    constexpr std::uint8_t computeChecksum8(std::span<const std::uint8_t> data)
    {
        std::uint8_t sum = 0;
        for (auto byte : data)
        {
            sum = static_cast<std::uint8_t>(sum + byte);
        }
        return sum;
    }

    // ========================================================================
    // Message framing
    // ========================================================================

    /// A framed message definition: header marker, length field, payload, optional CRC, trailer
    template<std::uint8_t HeaderByte, std::uint8_t TrailerByte, std::size_t MaxPayloadSize,
        bool HasCrc8 = false>
    struct FrameDefinition
    {
        static constexpr std::uint8_t header{ HeaderByte };
        static constexpr std::uint8_t trailer{ TrailerByte };
        static constexpr std::size_t maxPayload{ MaxPayloadSize };
        static constexpr bool hasCrc{ HasCrc8 };
        // Frame layout: [header(1)] [length(1)] [payload(N)] [crc8(0 or 1)] [trailer(1)]
        static constexpr std::size_t overhead{ 2 + (hasCrc ? 1 : 0) + 1 };
        static constexpr std::size_t maxFrameSize{ overhead + maxPayload };
    };

    /// Validate a framed message and extract payload span
    template<typename Frame>
    constexpr std::expected<std::span<const std::uint8_t>, FieldRanges::ParseError>
    parseFrame(std::span<const std::uint8_t> data)
    {
        if (data.size() < Frame::overhead)
            return std::unexpected(FieldRanges::ParseError::InvalidSize);

        if (data[0] != Frame::header)
            return std::unexpected(FieldRanges::ParseError::ValueNotExist);

        const std::size_t payloadLen = data[1];
        const std::size_t expectedSize = Frame::overhead + payloadLen;

        if (data.size() < expectedSize)
            return std::unexpected(FieldRanges::ParseError::InvalidSize);

        if (data[expectedSize - 1] != Frame::trailer)
            return std::unexpected(FieldRanges::ParseError::ValueNotExist);

        auto payload = data.subspan(2, payloadLen);

        if constexpr (Frame::hasCrc)
        {
            auto storedCrc = data[2 + payloadLen];
            if (computeCrc8(payload) != storedCrc)
                return std::unexpected(FieldRanges::ParseError::ChecksumMismatch);
        }

        return payload;
    }

    /// Build a framed message from payload bytes
    template<typename Frame, std::size_t PayloadSize>
    constexpr auto buildFrame(const std::array<std::uint8_t, PayloadSize> &payload)
    {
        constexpr std::size_t frameSize = Frame::overhead + PayloadSize;
        std::array<std::uint8_t, frameSize> frame{};
        frame[0] = Frame::header;
        frame[1] = static_cast<std::uint8_t>(PayloadSize);
        for (std::size_t i = 0; i < PayloadSize; ++i)
        {
            frame[2 + i] = payload[i];
        }
        if constexpr (Frame::hasCrc)
        {
            auto payloadSpan = std::span<const std::uint8_t>(frame.data() + 2, PayloadSize);
            frame[2 + PayloadSize] = computeCrc8(payloadSpan);
            frame[frameSize - 1] = Frame::trailer;
        }
        else
        {
            frame[frameSize - 1] = Frame::trailer;
        }
        return frame;
    }

    // ========================================================================
    // Message ID dispatch registry
    // ========================================================================

    /// A registry entry mapping a message ID to a parse function
    template<typename IdType, IdType Id, typename ParseFunc>
    struct MessageEntry
    {
        static constexpr IdType id{ Id };
        ParseFunc parser;
    };

    /// Compile-time message registry for dispatching by ID
    template<typename IdType, typename... Entries>
    struct MessageRegistry
    {
        std::tuple<Entries...> entries;

        constexpr MessageRegistry(Entries... e) : entries{ e... } {}

        /// Dispatch: find entry matching id, invoke its parser on the payload
        template<typename Payload>
        constexpr bool dispatch(IdType id, const Payload &payload) const
        {
            return dispatchImpl(id, payload, std::index_sequence_for<Entries...>{});
        }

    private:
        template<typename Payload, std::size_t... Is>
        constexpr bool dispatchImpl(IdType id, const Payload &payload, std::index_sequence<Is...>) const
        {
            bool found = false;
            ((std::get<Is>(entries).id == id ? (std::get<Is>(entries).parser(payload), found = true, 0) : 0), ...);
            return found;
        }
    };

    template<typename IdType, typename... Entries>
    constexpr auto makeRegistry(Entries... entries)
    {
        return MessageRegistry<IdType, Entries...>{ entries... };
    }

    // ========================================================================
    // Reflection / field iteration
    // ========================================================================

    /// Invoke a callable for each field in a parameter pack
    template<typename Func, typename... Fields>
    constexpr void forEachField(Func &&func, const Fields &...fields)
    {
        (func(fields), ...);
    }

    /// Invoke a callable with index for each field
    template<typename Func, typename... Fields>
    constexpr void forEachFieldIndexed(Func &&func, const Fields &...fields)
    {
        std::size_t idx = 0;
        ((func(idx++, fields)), ...);
    }

    /// Get the number of fields
    template<typename... Fields>
    consteval std::size_t fieldCount()
    {
        return sizeof...(Fields);
    }

    // ========================================================================
    // operator<< for ParseError
    // ========================================================================

    inline std::ostream &operator<<(std::ostream &os, FieldRanges::ParseError error)
    {
        return os << FieldRanges::to_string(error);
    }

    // ========================================================================
    // std::format integration
    // ========================================================================

}// namespace MessageParser

// std::formatter specialization for ParseError
template<>
struct std::formatter<FieldRanges::ParseError> : std::formatter<std::string_view>
{
    auto format(FieldRanges::ParseError error, std::format_context &ctx) const
    {
        return std::formatter<std::string_view>::format(FieldRanges::to_string(error), ctx);
    }
};
