#include <MessageParser.h>
#include <catch2/catch_all.hpp>
#include <format>
#include <functional>
#include <span>
#include <sstream>
#include <vector>

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
    enum class Cmd : std::uint8_t
    {
        Start,
        Stop
    };
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
    STATIC_REQUIRE_FALSE(MessageParser::convertByteType(msg.msg, val).has_value());
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
    REQUIRE_FALSE(MessageParser::convertByteType(msg.msg, val).has_value());
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

    for (Type tempValue = range.min; tempValue <= range.max; ++tempValue)
    {
        const Message msg = { tempValue };
        REQUIRE(MessageParser::convertByteType(msg.msg, val) == tempValue);
    }
}

TEST_CASE("MinMaxRange with exact min value succeeds")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{ 50 }, .max = uint8_t{ 200 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 50 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 50);
}

TEST_CASE("MinMaxRange with exact max value succeeds")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{ 50 }, .max = uint8_t{ 200 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 200 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 200);
}

TEST_CASE("MinMaxRange one below min returns BelowRange")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{ 50 }, .max = uint8_t{ 200 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 49 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
}

TEST_CASE("MinMaxRange one above max returns AboveRange")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint8_t{ 50 }, .max = uint8_t{ 200 } }>{};
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
        constexpr Message msg = { 0x0, 0x0, 0x0, 0x0 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE_FALSE(returnValue.has_value());
    }
    {
        constexpr Message msg = { 0x0, 0x0, 0x0, 0x1 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Warning);
    }
    {
        constexpr Message msg = { 0x0, 0x0, 0x0, 0x2 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Fatal);
    }
    {
        constexpr Message msg = { 0x0, 0x0, 0x0, 0x3 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE(returnValue == Error::Critical);
    }
    {
        constexpr Message msg = { 0x0, 0x0, 0x0, 0x4 };
        constexpr auto returnValue = MessageParser::convertByteType(msg.msg, val);
        STATIC_REQUIRE_FALSE(returnValue.has_value());
    }
}

// ===========================================================================
// ParseError variant tests
// ===========================================================================

TEST_CASE("MinMaxRange below range returns BelowRange error")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0x00, 0x00 };// 0
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
}

TEST_CASE("MinMaxRange above range returns AboveRange error")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = 1000, .max = 65000 }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 0xFF, 0xFF };// 65535 > 65000
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
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{ 10 }, uint8_t{ 20 }, uint8_t{ 30 } }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 20 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 20);
}

TEST_CASE("SpecificRange first boundary value is accepted")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{ 10 }, uint8_t{ 20 }, uint8_t{ 30 } }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 10 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 10);
}

TEST_CASE("SpecificRange last boundary value is accepted")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{ 10 }, uint8_t{ 20 }, uint8_t{ 30 } }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 30 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 30);
}

TEST_CASE("SpecificRange returns ValueNotExist when value is not in the set")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{ 10 }, uint8_t{ 20 }, uint8_t{ 30 } }>{};
    constexpr std::array<uint8_t, sizeof(Type)> msg = { 15 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE_FALSE(result.has_value());
    STATIC_REQUIRE(result.error() == FieldRanges::ParseError::ValueNotExist);
}

TEST_CASE("SpecificRange with enum values validates correctly")
{
    enum class Mode : uint8_t
    {
        Idle = 0,
        Run = 1,
        Halt = 5,
        Shutdown = 10
    };
    static constexpr auto val = FieldConfiguration<0, Mode, FieldRanges::SpecificRange{ Mode::Idle, Mode::Run, Mode::Shutdown }>{};

    {
        constexpr std::array<uint8_t, 1> msg = { 0 };// Idle
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == Mode::Idle);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 5 };// Halt - NOT in set
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE_FALSE(result.has_value());
        STATIC_REQUIRE(result.error() == FieldRanges::ParseError::ValueNotExist);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 10 };// Shutdown
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == Mode::Shutdown);
    }
}

TEST_CASE("SpecificRange with single value")
{
    using Type = uint8_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::SpecificRange{ uint8_t{ 42 } }>{};

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
    static constexpr auto val = FieldConfiguration<2, Type, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }>{};
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
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    // 0x1234 stored as { 0x34, 0x12 } in little-endian
    constexpr std::array<uint8_t, 2> msg = { 0x34, 0x12 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0x1234 });
}

TEST_CASE("Little-endian uint32_t field parsed correctly")
{
    using Type = uint32_t;
    static constexpr auto val = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint32_t{ 0 }, .max = uint32_t{ 0xFFFFFFFF } }, ByteOrder::LittleEndian>{};
    // 0x01020304 stored as { 0x04, 0x03, 0x02, 0x01 } in little-endian
    constexpr std::array<uint8_t, 4> msg = { 0x04, 0x03, 0x02, 0x01 };
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint32_t{ 0x01020304 });
}

TEST_CASE("Little-endian field at non-zero byte index")
{
    using Type = uint16_t;
    static constexpr auto val = FieldConfiguration<2, Type, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    constexpr std::array<uint8_t, 4> msg = { 0x00, 0x00, 0xCD, 0xAB };// 0xABCD in LE at index 2
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == uint16_t{ 0xABCD });
}

TEST_CASE("Little-endian vs big-endian produce different results for same bytes")
{
    using Type = uint16_t;
    static constexpr auto beFld = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::BigEndian>{};
    static constexpr auto leFld = FieldConfiguration<0, Type, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    constexpr std::array<uint8_t, 2> msg = { 0x01, 0x02 };

    constexpr auto beResult = MessageParser::convertByteType(msg, beFld);
    constexpr auto leResult = MessageParser::convertByteType(msg, leFld);
    STATIC_REQUIRE(beResult.has_value());
    STATIC_REQUIRE(leResult.has_value());
    STATIC_REQUIRE(*beResult == uint16_t{ 0x0102 });// 258
    STATIC_REQUIRE(*leResult == uint16_t{ 0x0201 });// 513
}

// ===========================================================================
// validateMessage tests
// ===========================================================================

TEST_CASE("validateMessage returns true when all fields are valid")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x10, 0x20 };
    STATIC_REQUIRE(MessageParser::validateMessage(msg, f1, f2) == true);
}

TEST_CASE("validateMessage returns false when any field is invalid")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 100 }, .max = uint8_t{ 200 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x01, 0x20 };// f1 value 1 < 100
    STATIC_REQUIRE(MessageParser::validateMessage(msg, f1, f2) == false);
}

// ===========================================================================
// convertAll tests
// ===========================================================================

TEST_CASE("convertAll returns tuple of results for all fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 100 } }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x42, 0x32 };

    constexpr auto results = MessageParser::convertAll(msg, f1, f2);
    STATIC_REQUIRE(std::get<0>(results).has_value());
    STATIC_REQUIRE(*std::get<0>(results) == 0x42);
    STATIC_REQUIRE(std::get<1>(results).has_value());
    STATIC_REQUIRE(*std::get<1>(results) == 0x32);
}

TEST_CASE("convertAll captures errors in individual fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 100 }, .max = uint8_t{ 200 } }>{};
    constexpr std::array<uint8_t, 2> msg = { 0x42, 0x01 };// f2 = 1 < 100

    constexpr auto results = MessageParser::convertAll(msg, f1, f2);
    STATIC_REQUIRE(std::get<0>(results).has_value());
    STATIC_REQUIRE_FALSE(std::get<1>(results).has_value());
    STATIC_REQUIRE(std::get<1>(results).error() == FieldRanges::ParseError::BelowRange);
}

// ===========================================================================
// Factory / schema API tests
// ===========================================================================

TEST_CASE("makeField creates field configuration equivalent to manual declaration")
{
    static constexpr auto field = MessageParser::makeField<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>();

    STATIC_REQUIRE(field.byteIndex == 0);
    STATIC_REQUIRE(field.byteLength == sizeof(uint16_t));
    STATIC_REQUIRE(field.byteOrder == ByteOrder::LittleEndian);

    constexpr std::array<uint8_t, 2> msg = { 0x34, 0x12 };
    constexpr auto parsed = MessageParser::convertByteType(msg, field);
    STATIC_REQUIRE(parsed.has_value());
    STATIC_REQUIRE(*parsed == uint16_t{ 0x1234 });
}

TEST_CASE("makeBitField creates bit-field configuration")
{
    static constexpr auto field = MessageParser::makeBitField<0, 4, 4, uint8_t>();

    constexpr std::array<uint8_t, 1> msg = { 0xAB };
    constexpr auto parsed = MessageParser::convertBitField(msg, field);
    STATIC_REQUIRE(parsed.has_value());
    STATIC_REQUIRE(*parsed == 11);
}

TEST_CASE("MessageSchema convertAll and validate work with array input")
{
    enum class Status : uint8_t
    {
        Ok = 0,
        Warn = 1,
        Err = 2
    };

    static constexpr auto status = MessageParser::makeField<0, Status, FieldRanges::MinMaxRange{ .min = Status::Ok, .max = Status::Err }>();
    static constexpr auto counter = MessageParser::makeField<1, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 1000 } }>();

    constexpr auto schema = MessageParser::makeSchema(status, counter);
    STATIC_REQUIRE(decltype(schema)::sizeBytes() == 3);

    constexpr std::array<uint8_t, 3> msg = { 0x01, 0x01, 0xF4 };// Warn, 500
    constexpr auto results = schema.convertAll(msg);

    STATIC_REQUIRE(schema.validate(msg));
    STATIC_REQUIRE(std::get<0>(results).has_value());
    STATIC_REQUIRE(*std::get<0>(results) == Status::Warn);
    STATIC_REQUIRE(std::get<1>(results).has_value());
    STATIC_REQUIRE(*std::get<1>(results) == uint16_t{ 500 });
}

TEST_CASE("MessageSchema validate works with span input")
{
    static constexpr auto a = MessageParser::makeField<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 10 }, .max = uint8_t{ 20 } }>();
    static constexpr auto b = MessageParser::makeField<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>();

    constexpr auto schema = MessageParser::makeSchema(a, b);

    std::array<uint8_t, 2> ok = { 15, 42 };
    std::array<uint8_t, 2> bad = { 9, 42 };

    std::span<const uint8_t> okSpan(ok);
    std::span<const uint8_t> badSpan(bad);

    REQUIRE(schema.validate(okSpan));
    REQUIRE_FALSE(schema.validate(badSpan));
}

TEST_CASE("MessageSchema parse forwards to parseMessage")
{
    static constexpr auto a = MessageParser::makeField<0, uint8_t>();
    static constexpr auto b = MessageParser::makeField<1, uint16_t>();

    constexpr auto schema = MessageParser::makeSchema(a, b);
    constexpr std::array<uint8_t, 3> msg = { 0x01, 0x00, 0x02 };

    // This should compile and execute without assertion, mirroring parseMessage behavior.
    schema.parse(msg);
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
    enum class Color : uint8_t
    {
        Red = 1,
        Green = 2,
        Blue = 3
    };
    STATIC_REQUIRE(MessageParser::toUnderlying(Color::Red) == 1);
    STATIC_REQUIRE(MessageParser::toUnderlying(Color::Blue) == 3);
}

TEST_CASE("toUnderlying returns same value for non-enum types")
{
    STATIC_REQUIRE(MessageParser::toUnderlying(uint8_t{ 42 }) == 42);
    STATIC_REQUIRE(MessageParser::toUnderlying(uint16_t{ 1000 }) == 1000);
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
    static constexpr auto val = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    STATIC_REQUIRE(val.byteOrder == ByteOrder::LittleEndian);
}

// ===========================================================================
// Multi-field message end-to-end test
// ===========================================================================

TEST_CASE("Full message parsing with multiple fields end-to-end")
{
    enum class Status : uint8_t
    {
        OK = 0,
        WARN = 1,
        ERR = 2
    };
    enum class Mode : uint8_t
    {
        Idle = 0,
        Active = 1,
        Sleep = 2
    };

    static constexpr auto statusField = FieldConfiguration<0, Status, FieldRanges::MinMaxRange{ .min = Status::OK, .max = Status::ERR }>{};
    static constexpr auto modeField = FieldConfiguration<1, Mode, FieldRanges::SpecificRange{ Mode::Idle, Mode::Active }>{};
    static constexpr auto counterField = FieldConfiguration<2, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 10000 } }>{};
    static constexpr auto flagField = FieldConfiguration<4, uint8_t, FieldRanges::SpecificRange{ uint8_t{ 0 }, uint8_t{ 1 } }>{};

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
    STATIC_REQUIRE(*counter == uint16_t{ 500 });

    constexpr auto flag = MessageParser::convertByteType(msg, flagField);
    STATIC_REQUIRE(flag.has_value());
    STATIC_REQUIRE(*flag == 1);

    STATIC_REQUIRE(MessageParser::validateMessage(msg, statusField, modeField, counterField, flagField));
}

// ===========================================================================
// Signed integer support tests
// ===========================================================================

TEST_CASE("Signed int8_t field parsed correctly")
{
    static constexpr auto val = FieldConfiguration<0, int8_t, FieldRanges::MinMaxRange{ .min = int8_t{ -128 }, .max = int8_t{ 127 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 0xFF };// -1 in two's complement
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == int8_t{ -1 });
}

TEST_CASE("Signed int16_t field big-endian")
{
    static constexpr auto val = FieldConfiguration<0, int16_t, FieldRanges::MinMaxRange{ .min = int16_t{ -32768 }, .max = int16_t{ 32767 } }>{};
    constexpr std::array<uint8_t, 2> msg = { 0xFF, 0xFE };// -2 in big-endian
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == int16_t{ -2 });
}

TEST_CASE("Signed int16_t field little-endian")
{
    static constexpr auto val = FieldConfiguration<0, int16_t, FieldRanges::MinMaxRange{ .min = int16_t{ -32768 }, .max = int16_t{ 32767 } }, ByteOrder::LittleEndian>{};
    constexpr std::array<uint8_t, 2> msg = { 0xFE, 0xFF };// -2 in little-endian
    constexpr auto result = MessageParser::convertByteType(msg, val);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == int16_t{ -2 });
}

TEST_CASE("Signed int8_t range validation works")
{
    static constexpr auto val = FieldConfiguration<0, int8_t, FieldRanges::MinMaxRange{ .min = int8_t{ -10 }, .max = int8_t{ 10 } }>{};
    {
        constexpr std::array<uint8_t, 1> msg = { 0x05 };// 5
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == int8_t{ 5 });
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 0x80 };// -128, below range
        constexpr auto result = MessageParser::convertByteType(msg, val);
        STATIC_REQUIRE_FALSE(result.has_value());
        STATIC_REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
    }
}

// ===========================================================================
// Floating-point field support tests
// ===========================================================================

TEST_CASE("Float field big-endian parse")
{
    // 3.14f = 0x4048F5C3 in IEEE 754
    constexpr std::array<uint8_t, 4> msg = { 0x40, 0x48, 0xF5, 0xC3 };
    constexpr auto result = MessageParser::getFloatField<0, ByteOrder::BigEndian>(msg);
    REQUIRE(result == Catch::Approx(3.14f).epsilon(0.001f));
}

TEST_CASE("Float field little-endian parse")
{
    constexpr std::array<uint8_t, 4> msg = { 0xC3, 0xF5, 0x48, 0x40 };
    constexpr auto result = MessageParser::getFloatField<0, ByteOrder::LittleEndian>(msg);
    REQUIRE(result == Catch::Approx(3.14f).epsilon(0.001f));
}

TEST_CASE("Double field big-endian parse")
{
    // 3.14 = 0x40091EB851EB851F in IEEE 754 double
    constexpr std::array<uint8_t, 8> msg = { 0x40, 0x09, 0x1E, 0xB8, 0x51, 0xEB, 0x85, 0x1F };
    constexpr auto result = MessageParser::getDoubleField<0, ByteOrder::BigEndian>(msg);
    REQUIRE(result == Catch::Approx(3.14).epsilon(0.0001));
}

TEST_CASE("Float field at non-zero index")
{
    constexpr std::array<uint8_t, 6> msg = { 0x00, 0x00, 0x40, 0x48, 0xF5, 0xC3 };
    constexpr auto result = MessageParser::getFloatField<2, ByteOrder::BigEndian>(msg);
    REQUIRE(result == Catch::Approx(3.14f).epsilon(0.001f));
}

// ===========================================================================
// Bit-field support tests
// ===========================================================================

TEST_CASE("BitField parse 3-bit field from bit offset 0")
{
    // Byte: 0b10110000 = 0xB0, bit offset 0, width 3 -> 101 = 5
    static constexpr auto bf = BitFieldConfiguration<0, 0, 3, uint8_t>{};
    constexpr std::array<uint8_t, 1> msg = { 0xB0 };
    constexpr auto result = MessageParser::convertBitField(msg, bf);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 5);
}

TEST_CASE("BitField parse 4-bit field from bit offset 4")
{
    // Byte: 0xAB = 0b10101011, bit offset 4, width 4 -> 1011 = 11
    static constexpr auto bf = BitFieldConfiguration<0, 4, 4, uint8_t>{};
    constexpr std::array<uint8_t, 1> msg = { 0xAB };
    constexpr auto result = MessageParser::convertBitField(msg, bf);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 11);
}

TEST_CASE("BitField spanning two bytes")
{
    // Bytes: 0x12 0x34 = 0b0001_0010 0011_0100
    // Bit offset 4, width 8 -> 0010_0011 = 0x23 = 35
    static constexpr auto bf = BitFieldConfiguration<0, 4, 8, uint8_t>{};
    constexpr std::array<uint8_t, 2> msg = { 0x12, 0x34 };
    constexpr auto result = MessageParser::convertBitField(msg, bf);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 0x23);
}

TEST_CASE("BitField with range validation")
{
    static constexpr auto bf = BitFieldConfiguration<0, 0, 4, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 7 } }>{};
    {
        constexpr std::array<uint8_t, 1> msg = { 0x50 };// top 4 bits = 0101 = 5
        constexpr auto result = MessageParser::convertBitField(msg, bf);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == 5);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 0xF0 };// top 4 bits = 1111 = 15 > 7
        constexpr auto result = MessageParser::convertBitField(msg, bf);
        STATIC_REQUIRE_FALSE(result.has_value());
        STATIC_REQUIRE(result.error() == FieldRanges::ParseError::AboveRange);
    }
}

// ===========================================================================
// Custom validator predicate tests
// ===========================================================================

TEST_CASE("CustomRange with predicate accepting even numbers")
{
    constexpr auto isEven = [](auto val) constexpr { return val % 2 == 0; };
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::CustomRange<uint8_t, decltype(isEven)>{ isEven }>{};

    {
        constexpr std::array<uint8_t, 1> msg = { 4 };
        constexpr auto result = MessageParser::convertByteType(msg, field);
        STATIC_REQUIRE(result.has_value());
        STATIC_REQUIRE(*result == 4);
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 3 };
        constexpr auto result = MessageParser::convertByteType(msg, field);
        STATIC_REQUIRE_FALSE(result.has_value());
        STATIC_REQUIRE(result.error() == FieldRanges::ParseError::CustomValidationFailed);
    }
}

TEST_CASE("CustomRange with predicate checking multiple of 4")
{
    constexpr auto isMultOf4 = [](auto val) constexpr { return val % 4 == 0; };
    static constexpr auto field = FieldConfiguration<0, uint8_t, FieldRanges::CustomRange<uint8_t, decltype(isMultOf4)>{ isMultOf4 }>{};

    {
        constexpr std::array<uint8_t, 1> msg = { 16 };
        constexpr auto result = MessageParser::convertByteType(msg, field);
        STATIC_REQUIRE(result.has_value());
    }
    {
        constexpr std::array<uint8_t, 1> msg = { 5 };
        constexpr auto result = MessageParser::convertByteType(msg, field);
        STATIC_REQUIRE_FALSE(result.has_value());
    }
}

// ===========================================================================
// Cross-field validation tests
// ===========================================================================

TEST_CASE("Cross-field validation passes with valid constraints")
{
    constexpr std::array<uint8_t, 3> msg = { 0x02, 0x00, 0x00 };// status=2 (error), rpm=0
    constexpr auto pred = [](const auto &m) constexpr {
        // if status > 1 (error), rpm must be 0
        auto status = MessageParser::convertByteType(m, FieldConfiguration<0, uint8_t>{});
        auto rpm = MessageParser::convertByteType(m, FieldConfiguration<1, uint16_t>{});
        if (status.has_value() && *status > 1)
            return rpm.has_value() && *rpm == 0;
        return true;
    };
    STATIC_REQUIRE(MessageParser::validateCrossField(msg, pred));
}

TEST_CASE("Cross-field validation fails with invalid constraints")
{
    constexpr std::array<uint8_t, 3> msg = { 0x02, 0x01, 0x00 };// status=2, rpm=256
    constexpr auto pred = [](const auto &m) constexpr {
        auto status = MessageParser::convertByteType(m, FieldConfiguration<0, uint8_t>{});
        auto rpm = MessageParser::convertByteType(m, FieldConfiguration<1, uint16_t>{});
        if (status.has_value() && *status > 1)
            return rpm.has_value() && *rpm == 0;
        return true;
    };
    STATIC_REQUIRE_FALSE(MessageParser::validateCrossField(msg, pred));
}

TEST_CASE("Multiple cross-field validators all pass")
{
    constexpr std::array<uint8_t, 3> msg = { 0x01, 0x00, 0x0A };
    constexpr auto pred1 = [](const auto &m) constexpr { return m[0] <= 5; };
    constexpr auto pred2 = [](const auto &m) constexpr { return m[2] > 0; };
    STATIC_REQUIRE(MessageParser::validateCrossFields(msg, pred1, pred2));
}

// ===========================================================================
// Optional/conditional field tests
// ===========================================================================

TEST_CASE("Conditional field parsed when condition is true")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 0x42 };
    auto result = MessageParser::convertByteTypeIf(msg, f, true);
    REQUIRE(result.has_value());
    REQUIRE(result->has_value());
    REQUIRE(**result == 0x42);
}

TEST_CASE("Conditional field returns nullopt when condition is false")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 0x42 };
    auto result = MessageParser::convertByteTypeIf(msg, f, false);
    REQUIRE(result.has_value());
    REQUIRE_FALSE(result->has_value());
}

TEST_CASE("Conditional field returns error when validation fails")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 100 }, .max = uint8_t{ 200 } }>{};
    constexpr std::array<uint8_t, 1> msg = { 0x01 };// 1 < 100
    auto result = MessageParser::convertByteTypeIf(msg, f, true);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == FieldRanges::ParseError::BelowRange);
}

// ===========================================================================
// Message serialization (encode) tests
// ===========================================================================

TEST_CASE("Encode single byte field")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t>{};
    std::array<uint8_t, 1> msg{};
    MessageParser::encodeField(msg, f, uint8_t{ 0x42 });
    REQUIRE(msg[0] == 0x42);
}

TEST_CASE("Encode uint16_t big-endian")
{
    static constexpr auto f = FieldConfiguration<0, uint16_t>{};
    std::array<uint8_t, 2> msg{};
    MessageParser::encodeField(msg, f, uint16_t{ 0x1234 });
    REQUIRE(msg[0] == 0x12);
    REQUIRE(msg[1] == 0x34);
}

TEST_CASE("Encode uint16_t little-endian")
{
    static constexpr auto f = FieldConfiguration<0, uint16_t, FieldRanges::MinMaxRange{ .min = uint16_t{ 0 }, .max = uint16_t{ 65535 } }, ByteOrder::LittleEndian>{};
    std::array<uint8_t, 2> msg{};
    MessageParser::encodeField(msg, f, uint16_t{ 0x1234 });
    REQUIRE(msg[0] == 0x34);
    REQUIRE(msg[1] == 0x12);
}

TEST_CASE("Encode enum field")
{
    enum class Status : uint8_t
    {
        OK = 0,
        ERR = 1
    };
    static constexpr auto f = FieldConfiguration<0, Status>{};
    std::array<uint8_t, 1> msg{};
    MessageParser::encodeField(msg, f, Status::ERR);
    REQUIRE(msg[0] == 1);
}

TEST_CASE("Encode uint32_t big-endian")
{
    static constexpr auto f = FieldConfiguration<0, uint32_t>{};
    std::array<uint8_t, 4> msg{};
    MessageParser::encodeField(msg, f, uint32_t{ 0xDEADBEEF });
    REQUIRE(msg[0] == 0xDE);
    REQUIRE(msg[1] == 0xAD);
    REQUIRE(msg[2] == 0xBE);
    REQUIRE(msg[3] == 0xEF);
}

TEST_CASE("Encode field at non-zero index")
{
    static constexpr auto f = FieldConfiguration<2, uint16_t>{};
    std::array<uint8_t, 4> msg{};
    MessageParser::encodeField(msg, f, uint16_t{ 0xABCD });
    REQUIRE(msg[0] == 0x00);
    REQUIRE(msg[1] == 0x00);
    REQUIRE(msg[2] == 0xAB);
    REQUIRE(msg[3] == 0xCD);
}

TEST_CASE("Encode with range check succeeds for valid value")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 10 }, .max = uint8_t{ 100 } }>{};
    std::array<uint8_t, 1> msg{};
    auto result = MessageParser::encodeFieldChecked(msg, f, uint8_t{ 50 });
    REQUIRE(result.has_value());
    REQUIRE(msg[0] == 50);
}

TEST_CASE("Encode with range check fails for invalid value")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 10 }, .max = uint8_t{ 100 } }>{};
    std::array<uint8_t, 1> msg{};
    auto result = MessageParser::encodeFieldChecked(msg, f, uint8_t{ 200 });
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == FieldRanges::ParseError::AboveRange);
}

TEST_CASE("Encode float big-endian")
{
    std::array<uint8_t, 4> msg{};
    MessageParser::encodeFloatField<0, ByteOrder::BigEndian>(msg, 3.14f);
    auto decoded = MessageParser::getFloatField<0, ByteOrder::BigEndian>(msg);
    REQUIRE(decoded == Catch::Approx(3.14f).epsilon(0.001f));
}

TEST_CASE("Encode float little-endian")
{
    std::array<uint8_t, 4> msg{};
    MessageParser::encodeFloatField<0, ByteOrder::LittleEndian>(msg, 2.718f);
    auto decoded = MessageParser::getFloatField<0, ByteOrder::LittleEndian>(msg);
    REQUIRE(decoded == Catch::Approx(2.718f).epsilon(0.001f));
}

TEST_CASE("Encode double big-endian")
{
    std::array<uint8_t, 8> msg{};
    MessageParser::encodeDoubleField<0, ByteOrder::BigEndian>(msg, 3.14159265);
    auto decoded = MessageParser::getDoubleField<0, ByteOrder::BigEndian>(msg);
    REQUIRE(decoded == Catch::Approx(3.14159265).epsilon(0.0000001));
}

TEST_CASE("Roundtrip encode-decode uint16_t preserves value")
{
    static constexpr auto f = FieldConfiguration<0, uint16_t>{};
    std::array<uint8_t, 2> msg{};
    MessageParser::encodeField(msg, f, uint16_t{ 12345 });
    auto decoded = MessageParser::convertByteType(msg, f);
    REQUIRE(decoded.has_value());
    REQUIRE(*decoded == 12345);
}

// ===========================================================================
// CRC / Checksum tests
// ===========================================================================

TEST_CASE("CRC-8 computation")
{
    std::array<uint8_t, 3> data = { 0x01, 0x02, 0x03 };
    auto crc = MessageParser::computeCrc8(std::span<const uint8_t>(data));
    // Just verify it produces a consistent value and is non-trivial
    REQUIRE(crc != 0);// known non-zero for this input
}

TEST_CASE("CRC-8 empty data returns 0")
{
    std::array<uint8_t, 0> data{};
    auto crc = MessageParser::computeCrc8(std::span<const uint8_t>(data.data(), 0));
    REQUIRE(crc == 0);
}

TEST_CASE("CRC-16 computation is consistent")
{
    std::array<uint8_t, 4> data = { 0xDE, 0xAD, 0xBE, 0xEF };
    auto crc1 = MessageParser::computeCrc16(std::span<const uint8_t>(data));
    auto crc2 = MessageParser::computeCrc16(std::span<const uint8_t>(data));
    REQUIRE(crc1 == crc2);
    REQUIRE(crc1 != 0);
}

TEST_CASE("CRC-32 known value")
{
    // "123456789" has CRC-32 = 0xCBF43926
    std::array<uint8_t, 9> data = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    auto crc = MessageParser::computeCrc32(std::span<const uint8_t>(data));
    REQUIRE(crc == 0xCBF43926);
}

TEST_CASE("Checksum8 computation")
{
    std::array<uint8_t, 3> data = { 0x10, 0x20, 0x30 };
    auto sum = MessageParser::computeChecksum8(std::span<const uint8_t>(data));
    REQUIRE(sum == 0x60);
}

TEST_CASE("verifyCrc8 passes for correct CRC")
{
    std::array<uint8_t, 3> payload = { 0x01, 0x02, 0x03 };
    auto expected_crc = MessageParser::computeCrc8(std::span<const uint8_t>(payload));
    std::array<uint8_t, 4> msg = { 0x01, 0x02, 0x03, expected_crc };
    auto result = MessageParser::verifyCrc8<0, 3, 3>(msg);
    REQUIRE(result.has_value());
}

TEST_CASE("verifyCrc8 fails for incorrect CRC")
{
    std::array<uint8_t, 4> msg = { 0x01, 0x02, 0x03, 0x00 };// wrong CRC
    auto result = MessageParser::verifyCrc8<0, 3, 3>(msg);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == FieldRanges::ParseError::ChecksumMismatch);
}

// ===========================================================================
// Message framing tests
// ===========================================================================

TEST_CASE("Build and parse frame without CRC")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 8, false>;
    std::array<uint8_t, 3> payload = { 0x01, 0x02, 0x03 };
    auto frame = MessageParser::buildFrame<Frame>(payload);
    REQUIRE(frame[0] == 0xAA);
    REQUIRE(frame[1] == 3);// length
    REQUIRE(frame[2] == 0x01);
    REQUIRE(frame[3] == 0x02);
    REQUIRE(frame[4] == 0x03);
    REQUIRE(frame[5] == 0x55);// trailer

    auto parsed = MessageParser::parseFrame<Frame>(std::span<const uint8_t>(frame));
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->size() == 3);
    REQUIRE((*parsed)[0] == 0x01);
    REQUIRE((*parsed)[1] == 0x02);
    REQUIRE((*parsed)[2] == 0x03);
}

TEST_CASE("Build and parse frame with CRC")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 8, true>;
    std::array<uint8_t, 2> payload = { 0x10, 0x20 };
    auto frame = MessageParser::buildFrame<Frame>(payload);
    REQUIRE(frame[0] == 0xAA);// header
    REQUIRE(frame[1] == 2);// length
    REQUIRE(frame[2] == 0x10);
    REQUIRE(frame[3] == 0x20);
    // frame[4] = CRC8 of payload
    REQUIRE(frame[5] == 0x55);// trailer

    auto parsed = MessageParser::parseFrame<Frame>(std::span<const uint8_t>(frame));
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->size() == 2);
}

TEST_CASE("Parse frame fails with wrong header")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 8, false>;
    std::array<uint8_t, 5> data = { 0xBB, 0x02, 0x01, 0x02, 0x55 };
    auto result = MessageParser::parseFrame<Frame>(std::span<const uint8_t>(data));
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == FieldRanges::ParseError::ValueNotExist);
}

TEST_CASE("Parse frame fails with insufficient size")
{
    using Frame = MessageParser::FrameDefinition<0xAA, 0x55, 8, false>;
    std::array<uint8_t, 2> data = { 0xAA, 0x05 };// claims 5 bytes payload but only 2 bytes total
    auto result = MessageParser::parseFrame<Frame>(std::span<const uint8_t>(data));
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == FieldRanges::ParseError::InvalidSize);
}

// ===========================================================================
// Message ID dispatch registry tests
// ===========================================================================

TEST_CASE("Message registry dispatches to correct handler")
{
    int result1 = 0;
    int result2 = 0;

    auto entry1 =
      MessageParser::MessageEntry<uint8_t, 0x01, std::function<void(std::span<const uint8_t>)>>{ .parser = [&result1](std::span<const uint8_t> payload) { result1 = payload[0]; } };
    auto entry2 =
      MessageParser::MessageEntry<uint8_t, 0x02, std::function<void(std::span<const uint8_t>)>>{ .parser = [&result2](std::span<const uint8_t> payload) { result2 = payload[0]; } };

    auto registry = MessageParser::makeRegistry<uint8_t>(entry1, entry2);
    std::array<uint8_t, 1> payload = { 0x42 };
    auto found = registry.dispatch(uint8_t{ 0x01 }, std::span<const uint8_t>(payload));
    REQUIRE(found);
    REQUIRE(result1 == 0x42);
    REQUIRE(result2 == 0);
}

TEST_CASE("Message registry returns false for unknown ID")
{
    auto entry1 = MessageParser::MessageEntry<uint8_t, 0x01, std::function<void(std::span<const uint8_t>)>>{ .parser = [](std::span<const uint8_t>) {} };
    auto registry = MessageParser::makeRegistry<uint8_t>(entry1);
    std::array<uint8_t, 1> payload = { 0x00 };
    auto found = registry.dispatch(uint8_t{ 0xFF }, std::span<const uint8_t>(payload));
    REQUIRE_FALSE(found);
}

// ===========================================================================
// std::span support tests
// ===========================================================================

TEST_CASE("convertByteType works with std::span")
{
    static constexpr auto f = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    std::array<uint8_t, 3> data = { 0x42, 0x10, 0x20 };
    std::span<const uint8_t> msg(data);
    auto result = MessageParser::convertByteType(msg, f);
    REQUIRE(result.has_value());
    REQUIRE(*result == 0x42);
}

TEST_CASE("convertByteType span multi-byte big-endian")
{
    static constexpr auto f = FieldConfiguration<0, uint16_t>{};
    std::array<uint8_t, 2> data = { 0x12, 0x34 };
    std::span<const uint8_t> msg(data);
    auto result = MessageParser::convertByteType(msg, f);
    REQUIRE(result.has_value());
    REQUIRE(*result == uint16_t{ 0x1234 });
}

TEST_CASE("validateMessage works with std::span")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    std::array<uint8_t, 2> data = { 0x10, 0x20 };
    std::span<const uint8_t> msg(data);
    REQUIRE(MessageParser::validateMessage(msg, f1, f2));
}

TEST_CASE("convertAll works with std::span")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 255 } }>{};
    static constexpr auto f2 = FieldConfiguration<1, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 100 } }>{};
    std::array<uint8_t, 2> data = { 0x42, 0x32 };
    std::span<const uint8_t> msg(data);
    auto results = MessageParser::convertAll(msg, f1, f2);
    REQUIRE(std::get<0>(results).has_value());
    REQUIRE(*std::get<0>(results) == 0x42);
    REQUIRE(std::get<1>(results).has_value());
    REQUIRE(*std::get<1>(results) == 0x32);
}

// ===========================================================================
// Named field tests
// ===========================================================================

TEST_CASE("NamedFieldConfiguration stores name")
{
    static constexpr auto f = NamedFieldConfiguration<0, uint8_t>{ .name = "temperature" };
    STATIC_REQUIRE(f.name == "temperature");
    STATIC_REQUIRE(f.byteIndex == 0);
    STATIC_REQUIRE(f.byteLength == 1);
}

TEST_CASE("NamedFieldConfiguration works with convertByteType")
{
    static constexpr auto f = NamedFieldConfiguration<0, uint8_t, FieldRanges::MinMaxRange{ .min = uint8_t{ 0 }, .max = uint8_t{ 100 } }>{ .name = "sensor" };
    constexpr std::array<uint8_t, 1> msg = { 50 };
    constexpr auto result = MessageParser::convertByteType(msg, f);
    STATIC_REQUIRE(result.has_value());
    STATIC_REQUIRE(*result == 50);
}

// ===========================================================================
// Reflection / field iteration tests
// ===========================================================================

TEST_CASE("forEachField invokes callable for each field")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};
    static constexpr auto f3 = FieldConfiguration<3, uint8_t>{};

    int count = 0;
    MessageParser::forEachField([&count](const auto &) { ++count; }, f1, f2, f3);
    REQUIRE(count == 3);
}

TEST_CASE("forEachFieldIndexed provides correct indices")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};

    std::vector<std::size_t> indices;
    MessageParser::forEachFieldIndexed([&indices](std::size_t idx, const auto &) { indices.push_back(idx); }, f1, f2);
    REQUIRE(indices.size() == 2);
    REQUIRE(indices[0] == 0);
    REQUIRE(indices[1] == 1);
}

TEST_CASE("fieldCount returns correct number")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};
    static constexpr auto f3 = FieldConfiguration<3, uint32_t>{};
    STATIC_REQUIRE(MessageParser::fieldCount<decltype(f1), decltype(f2), decltype(f3)>() == 3);
}

// ===========================================================================
// operator<< tests
// ===========================================================================

TEST_CASE("operator<< for ParseError produces correct output")
{
    std::ostringstream oss;
    oss << FieldRanges::ParseError::BelowRange;
    REQUIRE(oss.str() == "BelowRange");
}

// ===========================================================================
// std::format integration tests
// ===========================================================================

TEST_CASE("std::format formats ParseError correctly")
{
    auto str = std::format("{}", FieldRanges::ParseError::AboveRange);
    REQUIRE(str == "AboveRange");
}

TEST_CASE("std::format formats all ParseError values")
{
    REQUIRE(std::format("{}", FieldRanges::ParseError::ValueNotExist) == "ValueNotExist");
    REQUIRE(std::format("{}", FieldRanges::ParseError::BelowRange) == "BelowRange");
    REQUIRE(std::format("{}", FieldRanges::ParseError::AboveRange) == "AboveRange");
    REQUIRE(std::format("{}", FieldRanges::ParseError::InvalidSize) == "InvalidSize");
    REQUIRE(std::format("{}", FieldRanges::ParseError::ChecksumMismatch) == "ChecksumMismatch");
    REQUIRE(std::format("{}", FieldRanges::ParseError::CustomValidationFailed) == "CustomValidationFailed");
}

// ===========================================================================
// New ParseError to_string tests
// ===========================================================================

TEST_CASE("ParseError to_string includes new error types")
{
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::ChecksumMismatch) == "ChecksumMismatch");
    STATIC_REQUIRE(FieldRanges::to_string(FieldRanges::ParseError::CustomValidationFailed) == "CustomValidationFailed");
}
