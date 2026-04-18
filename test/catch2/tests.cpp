#include <MessageParser.h>
#include <catch2/catch_all.hpp>

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

// ---------------------------------------------------------------------------
// ParseError variant tests
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// SpecificRange tests
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Multi-byte parsing tests
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// getSize with 3+ fields
// ---------------------------------------------------------------------------

TEST_CASE("Calculate size with three fields")
{
    static constexpr auto f1 = FieldConfiguration<0, uint8_t>{};
    static constexpr auto f2 = FieldConfiguration<1, uint16_t>{};
    static constexpr auto f3 = FieldConfiguration<3, uint32_t>{};
    STATIC_REQUIRE(MessageParser::getSize<decltype(f1), decltype(f2), decltype(f3)>() == 7);
}

// ---------------------------------------------------------------------------
// getMessageSize
// ---------------------------------------------------------------------------

TEST_CASE("getMessageSize returns array size")
{
    constexpr std::array<uint8_t, 5> msg = {};
    STATIC_REQUIRE(MessageParser::getMessageSize(msg) == 5);
}
