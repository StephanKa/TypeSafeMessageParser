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
    STATIC_REQUIRE(MessageParser::convertByteType(msg, val) == std::nullopt);
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
    STATIC_REQUIRE(MessageParser::convertByteType(msg, val) == 43775);
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
    REQUIRE(MessageParser::convertByteType(msg, val) == std::nullopt);
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
        REQUIRE(MessageParser::convertByteType(msg, val) == tempValue);
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
    const auto returnValue = MessageParser::getField<decltype(val)>(msg);
    REQUIRE(returnValue == 0x42);
}
