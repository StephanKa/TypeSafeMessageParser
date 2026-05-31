#include <MessageParser.h>
#include <catch2/catch_all.hpp>

// ===========================================================================
// getSize tests
// ===========================================================================

TEST_CASE("Calculate size with one field")
{
    static constexpr auto val = FieldConfiguration<3, std::uint8_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
}

TEST_CASE("Calculate size with two fields")
{
    static constexpr auto val = FieldConfiguration<3, std::uint8_t>{};
    static constexpr auto val2 = FieldConfiguration<1, std::uint32_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val), decltype(val2)>() == (val.byteLength + val2.byteLength));
}

TEST_CASE("Calculate size with three fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};
    static constexpr auto f3 = FieldConfiguration<3, uint32_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(f1), decltype(f2), decltype(f3)>() == 7);
}

TEST_CASE("Calculate size with mixed enum and integer fields")
{
    enum class Cmd : std::uint8_t { Start, Stop };
    static constexpr auto f1 = FieldConfiguration<0, Cmd>{};
    static constexpr auto f2 = FieldConfiguration<1, uint32_t>{};
    static constexpr auto f3 = FieldConfiguration<5, uint16_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(f1), decltype(f2), decltype(f3)>() == 7);
}

// ===========================================================================
// getMessageSize tests
// ===========================================================================

TEST_CASE("getMessageSize returns array size")
{
    constexpr std::array<uint8_t, 5> msg = {};
    STATIC_REQUIRE(MessageParser::getMessageSize(msg) == 5);
}

TEST_CASE("getMessageSize returns 1 for single byte array")
{
    constexpr std::array<uint8_t, 1> msg = {};
    STATIC_REQUIRE(MessageParser::getMessageSize(msg) == 1);
}

// ===========================================================================
// parseMessage tests
// ===========================================================================

TEST_CASE("parseMessage succeeds when field sizes match message size")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};
    constexpr std::array<uint8_t, 3> msg = { 0x01, 0x02, 0x03 };
    // Should not assert - total field size (1+2) == array size (3)
    MessageParser::parseMessage(msg, f1, f2);
}

// ===========================================================================
// MinMaxRange tests
// ===========================================================================

TEST_CASE("Check MinMaxRange with below lower range data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = {};
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    STATIC_REQUIRE(MessageParser::convertByteType(msg.msg, val) == std::nullopt);
}

TEST_CASE("Check MinMaxRange with valid data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0xAA, 0xFF };
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    STATIC_REQUIRE(MessageParser::convertByteType(msg.msg, val) == 43775);
}

TEST_CASE("Check MinMaxRange with above upper range data")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0xFF, 0xFF };
    STATIC_REQUIRE(MessageParser::getSize<decltype(val)>() == val.byteLength);
    REQUIRE(MessageParser::convertByteType(msg.msg, val) == std::nullopt);
}

TEST_CASE("Check MinMaxRange complete range")
{
    using Type = uint8_t;
    constexpr FieldRanges::MinMaxRange range{ .min = 20, .max = 240 };
    static constexpr auto val = FieldConfiguration<0, Type, range>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    for(Type tempValue = range.min; tempValue <= range.max; ++tempValue)
    {
        const Message msg = { tempValue };
        REQUIRE(MessageParser::convertByteType(msg.msg, val) == tempValue);
    }
}

TEST_CASE("MinMaxRange with exact min value succeeds")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{50}, .max = uint8_t{200} }>{};
    constexpr std::array<uint8_t, 1> msg = { 50 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 50);
}

TEST_CASE("MinMaxRange with exact max value succeeds")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{50}, .max = uint8_t{200} }>{};
    constexpr std::array<uint8_t, 1> msg = { 200 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 200);
}

TEST_CASE("MinMaxRange one below min returns BelowRange")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{50}, .max = uint8_t{200} }>{};
    constexpr std::array<uint8_t, 1> msg = { 49 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
}

TEST_CASE("MinMaxRange one above max returns AboveRange")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{50}, .max = uint8_t{200} }>{};
    constexpr std::array<uint8_t, 1> msg = { 201 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::AboveRange);
}

// ===========================================================================
// getField tests
// ===========================================================================

TEST_CASE("Check getField")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0x42 };
    const auto returnValue = MessageParser::getField<decltype(val)>(msg.msg);
    REQUIRE(returnValue == 0x42);
}

TEST_CASE("Check getField with enum")
{
    enum class Error : std::uint8_t
    {
        Info,
        Warning,
        Fatal,
        Critical
    };
    using Type = Error;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    constexpr Message msg = { 0x2 };
    const auto returnValue = MessageParser::getField<decltype(val)>(msg.msg);
    REQUIRE(returnValue == Error::Fatal);
}

TEST_CASE("getField reads correct byte from non-zero index")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<2, Type>{};
    constexpr std::array<uint8_t, 4> msg = { 0x00, 0x00, 0xAB, 0x00 };
    constexpr auto result = MessageParser::getField<decltype(val)>(msg);
    STATIC_REQUIRE(result == 0xAB);
}

TEST_CASE("Check getField with enum and MinMaxRange")
{
    enum class Error
    {
        Info,
        Warning,
        Fatal,
        Critical,
        Unknown
    };
    using Type = Error;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = Error::Warning, .max = Error::Critical }>{};
    struct Message
    {
        std::array<uint8_t, sizeof(Type)> msg;
    };

    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x0 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == std::nullopt);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x1 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Warning);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x2 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Fatal);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x3 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Critical);
    }
    {
        constexpr Message msg = { 0x0 , 0x0, 0x0, 0x4 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == std::nullopt);
    }
}

// ===========================================================================
// ParseError variant tests
// ===========================================================================

TEST_CASE("MinMaxRange below range returns BelowRange error")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0x00, 0x00 }; // 0
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
}

TEST_CASE("MinMaxRange above range returns AboveRange error")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0xFF, 0xFF }; // 65535 > 65000
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::AboveRange);
}

// ===========================================================================
// SpecificRange tests
// ===========================================================================

TEST_CASE("SpecificRange returns value when it is in the allowed set")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{10}, uint8_t{20}, uint8_t{30} }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 20 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 20);
}

TEST_CASE("SpecificRange first boundary value is accepted")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{10}, uint8_t{20}, uint8_t{30} }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 10 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 10);
}

TEST_CASE("SpecificRange last boundary value is accepted")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{10}, uint8_t{20}, uint8_t{30} }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 30 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 30);
}

TEST_CASE("SpecificRange returns ValueNotExist when value is not in the set")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{10}, uint8_t{20}, uint8_t{30} }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 15 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::ValueNotExist);
}

TEST_CASE("SpecificRange with enum values validates correctly")
{
    enum class Mode : uint8_t { Idle = 0, Run = 1, Halt = 5, Shutdown = 10 };
    static constexpr auto val = FieldConfiguration<0, Mode, FieldRanges::SpecificRange{ Mode::Idle, Mode::Run, Mode::Shutdown }>{};

    {
        constexpr std::array<uint8_t, 1> msg = { 0 }; // Idle
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == Mode::Idle);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 5 }; // Halt - NOT in set
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE_FALSE(result.has_value());
        STATIC_REQUIRE(result.error() == FieldRanges::ParseError::ValueNotExist);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 10 }; // Shutdown
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == Mode::Shutdown);
    }
}

TEST_CASE("SpecificRange with single value")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{42} }>{};

    {
        constexpr std::array<uint8_t, 1> msg = { 42 };
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == 42);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 43 };
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE_FALSE(result.has_value());
    }
}

// ===========================================================================
// Multi-byte parsing tests (Big-Endian)
// ===========================================================================

TEST_CASE("Multi-byte uint32_t field parsed big-endian")
{
    using Type = uint32_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    // 0x01020304 in big-endian
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0x01, 0x02, 0x03, 0x04 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint32_t{ 0x01020304 });
}

TEST_CASE("Multi-byte field big-endian byte order: 0x0100 equals 256")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0x01, 0x00 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 256 });
}

TEST_CASE("Multi-byte field at non-zero byte index")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<2, Type, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }>{};
    // bytes 0-1 are padding, bytes 2-3 carry 0x1234
    constexpr std::array<uint8_t, 4> msg = { 0x00, 0x00, 0x12, 0x34 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0x1234 });
}

TEST_CASE("Multi-byte uint32_t maximum value")
{
    using Type = uint32_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    constexpr std::array<uint8_t, 4> msg = { 0xFF, 0xFF, 0xFF, 0xFF };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint32_t{ 0xFFFFFFFF });
}

TEST_CASE("Multi-byte uint16_t zero value")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type>{};
    constexpr std::array<uint8_t, 2> msg = { 0x00, 0x00 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0 });
}

// ===========================================================================
// Little-Endian multi-byte parsing tests
// ===========================================================================

TEST_CASE("Little-endian uint16_t field parsed correctly")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }, ByteOrder::LittleEndian>{};
    // 0x1234 stored as { 0x34, 0x12 } in little-endian
    constexpr std::array<uint8_t, 2> msg = { 0x34, 0x12 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0x1234 });
}

TEST_CASE("Little-endian uint32_t field parsed correctly")
{
    using Type = uint32_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint32_t{0}, .max = uint32_t{0xFFFFFFFF} }, ByteOrder::LittleEndian>{};
    // 0x01020304 stored as { 0x04, 0x03, 0x02, 0x01 } in little-endian
    constexpr std::array<uint8_t, 4> msg = { 0x04, 0x03, 0x02, 0x01 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint32_t{ 0x01020304 });
}

TEST_CASE("Little-endian field at non-zero byte index")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<2, Type, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }, ByteOrder::LittleEndian>{};
    constexpr std::array<uint8_t, 4> msg = { 0x00, 0x00, 0xCD, 0xAB }; // 0xABCD in LE at index 2
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0xABCD });
}

TEST_CASE("Little-endian vs big-endian produce different results for same bytes")
{
    using Type = uint16_t;
    static constexpr auto beFld = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }, ByteOrder::BigEndian>{};
    static constexpr auto leFld = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }, ByteOrder::LittleEndian>{};
    constexpr std::array<uint8_t, 2> msg = { 0x01, 0x02 };

    constexpr auto beResult = MessageParser::convertByteType(msg, beFld);
    constexpr auto leResult = MessageParser::convertByteType(msg, leFld);
    STATIC_REQUIRE(beResult.has_value());
    STATIC_REQUIRE(leResult.has_value());
    STATIC_REQUIRE(*beResult == uint16_t{ 0x0102 }); // 258
    STATIC_REQUIRE(*leResult == uint16_t{ 0x0201 }); // 513
}

// ===========================================================================
// validateMessage tests
// ===========================================================================

TEST_CASE("validateMessage returns true when all fields are valid")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{255} }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{255} }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x10, 0x20 };
    STATIC_REQUIRE(MessageParser::validateMessage(msg, f1, f2) == true);
}

TEST_CASE("validateMessage returns false when any field is invalid")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{100}, .max = uint8_t{200} }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{255} }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x01, 0x20 }; // f1 value 1 < 100
    STATIC_REQUIRE(MessageParser::validateMessage(msg, f1, f2) == false);
}

// ===========================================================================
// convertAll tests
// ===========================================================================

TEST_CASE("convertAll returns tuple of results for all fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{255} }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{100} }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x42, 0x32 };

    constexpr auto results = MessageParser::convertAll(msg, f1, f2);
    STATIC_REQUIRE(std::get<0>(results).has_value());
    STATIC_REQUIRE(*std::get<0>(results) == 0x42);
    STATIC_REQUIRE(std::get<1>(results).has_value());
    STATIC_REQUIRE(*std::get<1>(results) == 0x32);
}

TEST_CASE("convertAll captures errors in individual fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{0}, .max = uint8_t{255} }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{100}, .max = uint8_t{200} }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x42, 0x01 }; // f2 = 1 < 100

    constexpr auto results = MessageParser::convertAll(msg, f1, f2);
    STATIC_REQUIRE(std::get<0>(results).has_value());
    STATIC_REQUIRE_FALSE(std::get<1>(results).has_value());
    STATIC_REQUIRE(std::get<1>(results).error() == FieldRanges::ParseError::BelowRange);
}

// ===========================================================================
// to_string tests
// ===========================================================================

TEST_CASE("ParseError to_string returns correct strings")
{
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::ValueNotExist) == "ValueNotExist");
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::BelowRange) == "BelowRange");
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::AboveRange) == "AboveRange");
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::InvalidSize) == "InvalidSize");
}

// ===========================================================================
// toUnderlying tests
// ===========================================================================

TEST_CASE("toUnderlying converts enum to underlying type")
{
    enum class Color : uint8_t { Red = 1, Green = 2, Blue = 3 };
    STATIC_REQUIRE(MessageParser::toUnderlying(Color::Red) == 1);
    STATIC_REQUIRE(MessageParser::toUnderlying(Color::Blue) == 3);
}

TEST_CASE("toUnderlying returns same value for non-enum types")
{
    STATIC_REQUIRE(MessageParser::toUnderlying(uint8_t{42}) == 42);
    STATIC_REQUIRE(MessageParser::toUnderlying(uint16_t{1000}) == 1000);
}

// ===========================================================================
// IsFieldConfiguration concept tests
// ===========================================================================

TEST_CASE("IsFieldConfiguration concept accepts valid field configuration")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t>{};
    STATIC_REQUIRE(MessageParser::IsFieldConfiguration<decltype(f)>);
}

TEST_CASE("IsFieldConfiguration concept rejects non-field types")
{
    STATIC_REQUIRE_FALSE(MessageParser::IsFieldConfiguration<int>);
    STATIC_REQUIRE_FALSE(MessageParser::IsFieldConfiguration<std::array<uint8_t, 4>>);
}

// ===========================================================================
// ByteOrder enum value tests
// ===========================================================================

TEST_CASE("Default ByteOrder is BigEndian")
{
    static constexpr auto val = FieldConfiguration<0, uint16_t>{};
    STATIC_REQUIRE(val.byteOrder == ByteOrder::BigEndian);
}

TEST_CASE("Explicit LittleEndian ByteOrder is stored")
{
    static constexpr auto val = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{65535} }, ByteOrder::LittleEndian>{};
    STATIC_REQUIRE(val.byteOrder == ByteOrder::LittleEndian);
}

// ===========================================================================
// Multi-field message end-to-end test
// ===========================================================================

TEST_CASE("Full message parsing with multiple fields end-to-end")
{
    enum class Status : uint8_t { OK = 0, WARN = 1, ERR = 2 };
    enum class Mode : uint8_t { Idle = 0, Active = 1, Sleep = 2 };

    static constexpr auto statusField = FieldConfiguration<0, Status, FieldRanges::MinMaxRange{ .min = Status::OK, .max = Status::ERR }>{};
    static constexpr auto modeField = FieldConfiguration<1, Mode, FieldRanges::SpecificRange{ Mode::Idle, Mode::Active }>{};
    static constexpr auto counterField = FieldConfiguration<2, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{0}, .max = uint16_t{10000} }>{};
    static constexpr auto flagField = FieldConfiguration<4, uint8_t, FieldRanges::SpecificRange{ uint8_t{0}, uint8_t{1} }>{};

    constexpr std::array<uint8_t, 5> msg = { 0x01, 0x00, 0x01, 0xF4, 0x01 };
    // Status = WARN(1), Mode = Idle(0), Counter = 0x01F4 = 500, Flag = 1

    MessageParser::parseMessage(msg, statusField, modeField, counterField, flagField);

    constexpr auto status = MessageParser::convertByteType(msg, statusField);
    STATIC_REQUIRE(status.has_value());
    STATIC_REQUIRE(*status == Status::WARN);

    constexpr auto mode = MessageParser::convertByteType(msg, modeField);
    STATIC_REQUIRE(mode.has_value());
    STATIC_REQUIRE(*mode == Mode::Idle);

    constexpr auto counter = MessageParser::convertByteType(msg, counterField);
    STATIC_REQUIRE(counter.has_value());
    STATIC_REQUIRE(*counter == uint16_t{500});

    constexpr auto flag = MessageParser::convertByteType(msg, flagField);
    STATIC_REQUIRE(flag.has_value());
    STATIC_REQUIRE(*flag == 1);

    STATIC_REQUIRE(MessageParser::validateMessage(msg, statusField, modeField, counterField, flagField));
}
